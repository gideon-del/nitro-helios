#include <nitro-renderer/render-graph.h>
#include <imnodes.h>
#include <imgui.h>

namespace nitro::renderer
{
    RGTextureID RenderGraph::declareTexture(RGTextureDesc desc)
    {
        RGTextureID id = ++m_nextId;

        m_textures[id] = std::move(desc);
        return id;
    };
    RGBufferID RenderGraph::declareBuffer(RGBufferDesc desc)
    {
        RGBufferID id = ++m_nextId;

        m_buffers[id] = std::move(desc);
        return id;
    };

    void RenderGraph::addPass(RenderPass pass)
    {
        m_passes.push_back(std::move(pass));
    }

    void RenderGraph::compile()
    {
        m_depGraph = Graph();

        std::unordered_map<RGTextureID, int> textureProducerOf;
        std::unordered_map<RGTextureID, std::vector<int>> textureExtenderOf;
        std::unordered_map<RGBufferID, int> bufferProducerOf;
        std::unordered_map<RGBufferID, std::vector<int>> bufferExtenderOf;
        for (int i = 0; i < m_passes.size(); i++)
        {
            m_depGraph.addNode(i);

            for (auto &tid : m_passes[i].writes)
            {
                if (tid.writeMode == WriteMode::Producer)
                {
                    if (textureProducerOf.count(tid.id))
                    {
                        throw std::runtime_error("Texture " + m_textures[tid.id].name + " has two producers");
                    }

                    textureProducerOf[tid.id] = i;
                }
                else
                {
                    textureExtenderOf[tid.id].push_back(i);
                }
            }
            for (auto &bid : m_passes[i].writeBufs)
            {
                if (bid.writeMode == WriteMode::Producer)
                {
                    if (bufferProducerOf.count(bid.id))
                    {
                        throw std::runtime_error("Buffer " + m_buffers[bid.id].name + " has two producers");
                    }

                    bufferProducerOf[bid.id] = i;
                }
                else
                {
                    bufferExtenderOf[bid.id].push_back(i);
                }
            }
        }

        std::unordered_map<RGTextureID, int> textureLastWriter = textureProducerOf;

        for (auto &[tid, extendIndices] : textureExtenderOf)
        {
            if (!textureProducerOf.count(tid))
                throw std::runtime_error("Texture " + m_textures[tid].name + " has Extend with no Produce");

            std::sort(extendIndices.begin(), extendIndices.end());

            int prev = textureProducerOf[tid];

            for (auto &extendIdx : extendIndices)
            {

                m_depGraph.addEdge(prev, extendIdx);
                prev = extendIdx;
            }

            textureLastWriter[tid] = prev;
        }
        std::unordered_map<RGBufferID, int> bufferLastWriter = bufferProducerOf;
        for (auto &[bid, extendIndices] : bufferExtenderOf)
        {
            if (!bufferProducerOf.count(bid))
                throw std::runtime_error("Buffer " + m_buffers[bid].name + " has Extend with no Produce");

            std::sort(extendIndices.begin(), extendIndices.end());

            int prev = bufferProducerOf[bid];

            for (auto &extendIdx : extendIndices)
            {

                m_depGraph.addEdge(prev, extendIdx);
                prev = extendIdx;
            }

            bufferLastWriter[bid] = prev;
        }

        for (int i = 0; i < m_passes.size(); i++)
        {

            for (auto &tid : m_passes[i].reads)
            {
                if (!textureLastWriter.count(tid.id))
                {
                    throw std::runtime_error("Texture " + m_textures[tid.id].name + " has no producer");
                }

                int producer = textureLastWriter[tid.id];
                if (producer != i)
                {
                    m_depGraph.addEdge(producer, i);
                }
            }
            for (auto &bid : m_passes[i].readBufs)
            {
                if (!bufferLastWriter.count(bid.id))
                {
                    throw std::runtime_error("Buffer " + m_buffers[bid.id].name + " has no producer");
                }

                int producer = bufferLastWriter[bid.id];
                if (producer != i)
                {
                    m_depGraph.addEdge(producer, i);
                }
            }
        }

        m_executionOrder = m_depGraph.topoSort();

        auto errors = validate();

        if (!errors.empty())
        {
            for (auto &e : errors)
                std::cerr << "[RenderGraph] " << e.passName
                          << " / " << e.textureName
                          << ": " << e.message << "\n";
            throw std::runtime_error("Render graph validation failed");
        }
    }

    void RenderGraph::dryRun()
    {
        auto order = m_depGraph.topoSort();

        std::cout << "=== Render Graph Dry Run ===\n";

        std::cout << "Passes: " << order.size() << "\n";

        for (int i = 0; i < (int)order.size(); i++)
        {
            auto &pass = m_passes[order[i]];

            std::cout << i + 1 << ". " << pass.name << "\n";

            if (!pass.reads.empty())
            {
                std::cout << "  Texture Reads:  ";
                for (auto &read : pass.reads)
                    std::cout << m_textures.at(read.id).name << " ";
                std::cout << "\n";
            }
            if (!pass.readBufs.empty())
            {
                std::cout << "  Buffer Reads:  ";
                for (auto &read : pass.readBufs)
                    std::cout << m_buffers.at(read.id).name << " ";
                std::cout << "\n";
            }
            if (!pass.writes.empty())
            {
                std::cout << "  Texture Writes:  ";
                for (auto &write : pass.writes)
                    std::cout << m_textures.at(write.id).name << " ";
                std::cout << "\n";
            }
            if (!pass.writeBufs.empty())
            {
                std::cout << "  Buffer Writes:  ";
                for (auto &write : pass.writeBufs)
                    std::cout << m_buffers.at(write.id).name << " ";
                std::cout << "\n";
            }
        }
        std::cout << "\n";
    }

    void RenderGraph::allocateTextures(std::shared_ptr<rhi::RHIDevice> device, uint32_t frameWidth, uint32_t frameHeight)
    {

        device->waitIdle();
        deleteTextures(device);
        m_memoryBlocks.clear();
        m_aliasAssignments = computeAliases(frameWidth, frameHeight);
        rhi::RHICommandBuffer *cmd = device->createCommandBuffer();
        std::unordered_map<int, std::vector<RGResourceID>> blockMap;

        for (auto &[tid, blockId] : m_aliasAssignments)
        {
            blockMap[blockId].push_back(tid);
        }

        auto makeTextureDesc = [&](RGTextureID tid, bool isAlias = false)
        {
            auto &desc = m_textures[tid];
            rhi::TextureDesc textureDesc;
            textureDesc.usage = rhi::TextureDesc::Usage::ShaderRead;
            textureDesc.format = desc.format;
            if (isDepthFormat(desc.format))
            {
                textureDesc.usage = rhi::TextureDesc::Usage::ShaderRead | rhi::TextureDesc::Usage::DepthStencil;
            }
            else if (desc.isStorage)
            {
                textureDesc.usage = rhi::TextureDesc::Usage::Storage | rhi::TextureDesc::Usage::ShaderRead;
            }
            else if (isWrittenByAnyPass(tid))
            {
                textureDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;
            }

            textureDesc.size.width = desc.width ? desc.width : frameWidth;
            textureDesc.size.height = desc.height ? desc.height : frameHeight;
            textureDesc.isAliased = isAlias;
            return textureDesc;
        };

        auto alignUp = [&](size_t size, size_t alignment)
        {
            return (size + alignment - 1) & ~(alignment - 1);
        };
        for (auto &[blockId, textureIds] : blockMap)
        {

            if (textureIds.size() == 1)
            {
                auto &tid = textureIds[0];
                if (!m_allocatedTextures.count(textureIds[0]))
                {
                    auto textureDesc = makeTextureDesc(tid);
                    m_allocatedTextures[tid] = device->createTexture(textureDesc);
                    rhi::TextureBarrier textureBarrier;
                    textureBarrier.texture = m_allocatedTextures[tid];
                    textureBarrier.before = rhi::ResourceState::Undefined;
                    textureBarrier.after = rhi::ResourceState::ShaderRead;
                    cmd->textureBarrier(textureBarrier);
                }
                continue;
            }

            size_t blockSize = 0;
            size_t maxAlignment = 0;
            uint32_t combinedMemoryTypeBits = ~0u;

            for (auto &tid : textureIds)
            {
                auto req = device->textureMemoryRequirements(makeTextureDesc(tid, true));

                blockSize = std::max(blockSize, req.size);
                maxAlignment = std::max(maxAlignment, req.alignment);

                combinedMemoryTypeBits &= req.memoryTypeBits;
            }
            if (combinedMemoryTypeBits == 0)
            {
                throw std::runtime_error("Alias group has no compatible memory type — cannot share a heap");
            }

            size_t alignedBlockSize = alignUp(blockSize, maxAlignment);
            auto &block = m_memoryBlocks[blockId];
            if (block.heap)
            {
                device->destroyHeap(block.heap);
            }

            block.heap = device->createHeap(alignedBlockSize, combinedMemoryTypeBits);

            for (auto &tid : textureIds)
            {
                if (!m_allocatedTextures.count(tid))
                {
                    auto textureDesc = makeTextureDesc(tid, true);
                    m_allocatedTextures[tid] = device->createTextureFromHeap(block.heap, textureDesc, 0);
                    rhi::TextureBarrier textureBarrier;
                    textureBarrier.texture = m_allocatedTextures[tid];
                    textureBarrier.before = rhi::ResourceState::Undefined;
                    textureBarrier.after = rhi::ResourceState::ShaderRead;
                    cmd->textureBarrier(textureBarrier);
                }
            }
        };
        device->endCommandBuffer(cmd);
    }
    void RenderGraph::reallocateFrameTextures(std::shared_ptr<rhi::RHIDevice> device, uint32_t frameWidth, uint32_t frameHeight)
    {
        device->waitIdle();
        rhi::RHICommandBuffer *cmd = device->createCommandBuffer();
        for (auto &[tid, desc] : m_textures)
        {

            if (desc.width != 0 || desc.height != 0)
                continue;
            rhi::TextureDesc textureDesc;
            textureDesc.usage = rhi::TextureDesc::Usage::ShaderRead;
            textureDesc.format = desc.format;
            if (isDepthFormat(desc.format))
            {
                textureDesc.usage = rhi::TextureDesc::Usage::ShaderRead | rhi::TextureDesc::Usage::DepthStencil;
            }
            else if (desc.isStorage)
            {
                textureDesc.usage = rhi::TextureDesc::Usage::Storage | rhi::TextureDesc::Usage::ShaderRead;
            }
            else if (isWrittenByAnyPass(tid))
            {
                textureDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;
            }

            textureDesc.size.width = frameWidth;
            textureDesc.size.height = frameHeight;

            if (m_allocatedTextures.count(tid))
            {
                device->destroyTexture(m_allocatedTextures[tid]);
            }

            m_allocatedTextures[tid] = device->createTexture(textureDesc);

            rhi::TextureBarrier textureBarrier;
            textureBarrier.texture = m_allocatedTextures[tid];
            textureBarrier.before = rhi::ResourceState::Undefined;
            textureBarrier.after = rhi::ResourceState::ShaderRead;
            cmd->textureBarrier(textureBarrier);
        };
        device->endCommandBuffer(cmd);
    }

    void RenderGraph::allocateBuffers(std::shared_ptr<rhi::RHIDevice> device)
    {
        for (auto &[bid, desc] : m_buffers)
        {
            rhi::BufferDesc bufferDesc;

            bufferDesc.size = desc.size;
            bufferDesc.usage = desc.usage;
            bufferDesc.storage = isSharedStorageBuffer(bid) ? rhi::BufferDesc::StorageMode::Shared : rhi::BufferDesc::StorageMode::GPU;

            if (!m_allocatedBuffers.count(bid))
            {
                m_allocatedBuffers[bid] = device->createBuffer(bufferDesc);
            }
        };
    }
    void RenderGraph::deleteTextures(std::shared_ptr<rhi::RHIDevice> device)
    {
        for (auto [_, texture] : m_allocatedTextures)
        {
            device->destroyTexture(texture);
        }

        m_allocatedTextures.clear();

        for (auto &block : m_memoryBlocks)
        {
            if (block.heap)
            {
                device->destroyHeap(block.heap);
                block.heap = nullptr;
            }
        }
    }

    void RenderGraph::deleteBuffers(std::shared_ptr<rhi::RHIDevice> device)
    {
        for (auto [_, buffer] : m_allocatedBuffers)
        {
            device->destroyBuffer(buffer);
        }

        m_allocatedBuffers.clear();
    }

    bool RenderGraph::isDepthFormat(rhi::TextureDesc::ImageFormat format)
    {
        return format == rhi::TextureDesc::ImageFormat::Depth32Float || format == rhi::TextureDesc::ImageFormat::Depth32FloatStencil8;
    }
    bool RenderGraph::isWrittenByAnyPass(RGResourceID id)
    {
        for (auto &pass : m_passes)
        {
            for (auto &tid : pass.writes)
            {
                if (tid.id == id)
                {
                    return true;
                }
            }
            for (auto &bid : pass.writeBufs)
            {
                if (bid.id == id)
                {
                    return true;
                }
            }
        }

        return false;
    }
    bool RenderGraph::isSharedStorageBuffer(RGResourceID id)
    {
        if (!m_buffers.count(id))
            return false;

        return m_buffers[id].usage == rhi::BufferDesc::Usage::TransferDst || m_buffers[id].usage == rhi::BufferDesc::Usage::Uniform || m_buffers[id].usage == rhi::BufferDesc::Usage::Storage;
    }
    const RGResources RenderGraph::buildResources()
    {
        return RGResources{m_allocatedTextures, m_allocatedBuffers};
    }

    void RenderGraph::bindPassResources(const RGResources &resources)
    {

        for (auto &pass : m_passes)
        {
            pass.bind(resources);
        }
    }
    void RenderGraph::executeFrameGraph(const RGCompiledFrameGraph &compiledGraph, rhi::RHICommandBuffer *cmd, const RenderContext &ctx, RendererSettings &settings, rhi::RHITimer *timer)
    {

        auto resources = buildResources();
        for (auto &step : compiledGraph.steps)
        {

            std::visit(
                overloaded{
                    [&](const RGTextureBarrier &barrier)
                    {
                        // std::cout << "[RGTextureBarrier] " << m_textures[barrier.textureId].name << " From " << std::to_string((int)barrier.from) << " To " << std::to_string((int)barrier.to) << "\n";
                        rhi::TextureBarrier textureBarrier;
                        textureBarrier.texture = resources.getTexture(barrier.textureId);
                        textureBarrier.before = barrier.from;
                        textureBarrier.after = barrier.to;

                        cmd->textureBarrier(textureBarrier);
                    },
                    [&](const int &passIdx)
                    {
                        auto &pass = m_passes[passIdx];
                        timer->begin(cmd, pass.name);
                        pass.execute(cmd, resources, ctx, settings);
                        timer->end(cmd, pass.name);
                    }},
                step);
        }
    }

    std::vector<RGValidationError> RenderGraph::validate()
    {
        std::vector<RGValidationError> errors;
        auto cycles = m_depGraph.findCycles();

        for (auto &cycle : cycles)
        {
            std::string path = "";
            for (auto &idx : cycle)
            {
                path += m_passes[idx].name + " → ";
            }

            errors.push_back({"", "", "Cycle Detected " + path});
        }

        std::unordered_map<RGTextureID, std::string> writerOf;

        for (auto &pass : m_passes)
        {
            for (auto &write : pass.writes)
            {
                writerOf[write.id] = pass.name;
            }
        }
        for (auto &pass : m_passes)
        {
            for (auto &write : pass.reads)
            {
                if (!writerOf.count(write.id))
                {
                    errors.push_back({pass.name,
                                      m_textures.at(write.id).name,
                                      "reads texture with no writer"});
                }
            }
        }

        return errors;
    }

    void RenderGraph::drawImGui()
    {
        ImGui::Begin("Render Graph");
        ImNodes::BeginNodeEditor();

        std::unordered_map<RGTextureID, int> writerOf;
        std::unordered_map<RGTextureID, std::vector<int>> readersOf;
        for (int passIdx = 0; passIdx < m_passes.size(); passIdx++)
        {
            auto &pass = m_passes[passIdx];

            ImNodes::BeginNode(passIdx);
            ImNodes::BeginNodeTitleBar();

            ImGui::TextUnformatted(pass.name.c_str());

            ImNodes::EndNodeTitleBar();
            for (int i = 0; i < (int)pass.reads.size(); i++)
            {
                int pinId = passIdx * 100 + i;
                ImNodes::BeginInputAttribute(pinId);
                ImGui::Text("%s", m_textures.at(pass.reads[i].id).name.c_str());
                ImNodes::EndInputAttribute();

                readersOf[pass.reads[i].id].push_back(passIdx);
            }

            for (int i = 0; i < (int)pass.writes.size(); i++)
            {
                int pinId = passIdx * 100 + 50 + i;
                ImNodes::BeginOutputAttribute(pinId);
                ImGui::Text("%s", m_textures.at(pass.writes[i].id).name.c_str());
                ImNodes::EndOutputAttribute();

                writerOf[pass.writes[i].id] = passIdx;
            }
            ImNodes::EndNode();
        }

        int edgeId = 0;
        for (auto &[tid, writerPassIdx] : writerOf)
        {
            int outputPin = writerPassIdx * 100 + 50 + writerOutputIndex(writerPassIdx, tid);
            for (int readerPassIdx : readersOf[tid])
            {
                int inputPin = readerPassIdx * 100 + readerInputIndex(readerPassIdx, tid);
                ImNodes::Link(edgeId++, outputPin, inputPin);
            }
        }

        ImNodes::EndNodeEditor();
        ImGui::End();

        drawLifetimeTimeline();
    }
    int RenderGraph::writerOutputIndex(int passIdx, RGTextureID tid)
    {
        auto &writes = m_passes[passIdx].writes;
        for (int i = 0; i < (int)writes.size(); i++)
            if (writes[i].id == tid)
                return i;
        return -1;
    }

    int RenderGraph::readerInputIndex(int passIdx, RGTextureID tid)
    {
        auto &reads = m_passes[passIdx].reads;
        for (int i = 0; i < (int)reads.size(); i++)
            if (reads[i].id == tid)
                return i;
        return -1;
    }

    std::unordered_map<RGResourceID, RGLifetime> RenderGraph::computeLifetimes()
    {
        std::unordered_map<RGResourceID, RGLifetime> lifetime;

        for (int i = 0; i < m_executionOrder.size(); i++)
        {
            auto &pass = m_passes[m_executionOrder[i]];

            for (auto &tid : pass.writes)
            {
                lifetime[tid.id].createdAt = i;
                lifetime[tid.id].lastUsedAt = i;
                lifetime[tid.id].id = tid.id;
            };
            for (auto &tid : pass.reads)
            {

                lifetime[tid.id].lastUsedAt = std::max(lifetime[tid.id].lastUsedAt, i);
                lifetime[tid.id].id = tid.id;
            };
        }

        return lifetime;
    }
    void RenderGraph::drawLifetimeTimeline()
    {
        auto lifetimes = computeLifetimes();

        std::vector<RGResourceID> ids;
        for (auto &[tid, lt] : lifetimes)
            ids.push_back(tid);
        std::sort(ids.begin(), ids.end(), [&](RGResourceID a, RGResourceID b)
                  { return lifetimes[a].createdAt < lifetimes[b].createdAt; });

        ImGui::Begin("Lifetimes");
        if (ImGui::BeginTable("lifetimes_table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Resource");
            ImGui::TableSetupColumn("Created at");
            ImGui::TableSetupColumn("Last used at");
            ImGui::TableSetupColumn("Span");
            ImGui::TableSetupColumn("Memory Block");
            ImGui::TableHeadersRow();

            for (auto tid : ids)
            {
                auto &lt = lifetimes[tid];
                auto &desc = m_textures[tid];

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(m_textures.at(tid).name.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(m_passes[m_executionOrder[lt.createdAt]].name.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(m_passes[m_executionOrder[lt.lastUsedAt]].name.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%d", lt.lastUsedAt - lt.createdAt + 1);
                ImGui::TableNextColumn();
                ImGui::Text("%d", desc.transient ? m_aliasAssignments[tid] : -1);
            }
            ImGui::EndTable();
        }
        ImGui::End();
    }

    std::unordered_map<RGResourceID, int> RenderGraph::computeAliases(uint32_t frameWidth, uint32_t frameHeight)
    {
        auto lifetimeMap = computeLifetimes();
        std::vector<RGLifetime> lifetimes;
        for (auto &[tid, lt] : lifetimeMap)
            lifetimes.push_back(lt);

        std::sort(lifetimes.begin(), lifetimes.end(), [](RGLifetime a, RGLifetime b)
                  { return a.createdAt < b.createdAt; });
        std::unordered_map<RGResourceID, int> m_aliasAssignments;
        for (auto &lt : lifetimes)
        {
            auto &desc = m_textures[lt.id];
            uint32_t w = desc.width ? desc.width : frameWidth;
            uint32_t h = desc.height ? desc.height : frameHeight;
            size_t neededSize = (size_t)w * h * rhi::getImageFormatSize(desc.format);
            if (!desc.transient)
            {
                auto blockId = static_cast<int>(m_memoryBlocks.size());
                m_memoryBlocks.push_back({blockId, neededSize, static_cast<int>(lifetimes.size() - 1)});
                continue;
            };

            int reusedBlock = -1;

            for (auto &block : m_memoryBlocks)
            {
                if (block.size >= neededSize && block.lastUsePass < lt.createdAt)
                {
                    if (reusedBlock < 0 || m_memoryBlocks[reusedBlock].size > block.size)
                    {
                        reusedBlock = block.id;
                    }
                }
            }

            if (reusedBlock >= 0)
            {
                m_aliasAssignments[lt.id] = reusedBlock;
                m_memoryBlocks[reusedBlock].lastUsePass = lt.lastUsedAt;
            }
            else
            {
                auto blockId = static_cast<int>(m_memoryBlocks.size());

                m_memoryBlocks.push_back({blockId, neededSize, lt.lastUsedAt});

                m_aliasAssignments[lt.id] = blockId;
            }
        };

        return m_aliasAssignments;
    };

    const RGCompiledFrameGraph RenderGraph::compileFrameGraph()
    {
        std::unordered_map<RGTextureID, rhi::ResourceState> currentLayout;
        RGCompiledFrameGraph compiledGraph;

        auto emitBarrierIfNeeded = [&](const RGResourceAccess &access)
        {
            auto it = currentLayout.find(access.id);
            rhi::ResourceState previousState = (it != currentLayout.end())
                                                   ? it->second
                                                   : rhi::ResourceState::ShaderRead;

            if (previousState != access.state)
                compiledGraph.steps.push_back(RGTextureBarrier{access.id, previousState, access.state});

            currentLayout[access.id] = access.state;
        };
        for (auto &passIdx : m_executionOrder)
        {
            auto &pass = m_passes[passIdx];
            for (auto &read : pass.reads)
            {
                emitBarrierIfNeeded(read);
            }
            for (auto &write : pass.writes)
            {

                emitBarrierIfNeeded(write);
            }

            compiledGraph.steps.push_back((int)passIdx);
        }

        return compiledGraph;
    }
} // namespace nitro::renderer
