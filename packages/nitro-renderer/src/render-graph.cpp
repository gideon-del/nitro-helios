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
        std::unordered_map<RGBufferID, int> bufferProducerOf;
        for (int i = 0; i < m_passes.size(); i++)
        {
            m_depGraph.addNode(i);

            for (auto &tid : m_passes[i].writes)
            {
                if (textureProducerOf.count(tid))
                {
                    throw std::runtime_error("Texture " + m_textures[tid].name + " has two producers");
                }

                textureProducerOf[tid] = i;
            }
            for (auto &bid : m_passes[i].writeBufs)
            {
                if (bufferProducerOf.count(bid))
                {
                    throw std::runtime_error("Buffer " + m_buffers[bid].name + " has two producers");
                }

                bufferProducerOf[bid] = i;
            }
        }

        for (int i = 0; i < m_passes.size(); i++)
        {

            for (auto &tid : m_passes[i].reads)
            {
                if (!textureProducerOf.count(tid))
                {
                    throw std::runtime_error("Texture " + m_textures[tid].name + " has no producer");
                }

                int producer = textureProducerOf[tid];
                if (producer != i)
                {
                    m_depGraph.addEdge(producer, i);
                }
            }
            for (auto &bid : m_passes[i].readBufs)
            {
                if (!bufferProducerOf.count(bid))
                {
                    throw std::runtime_error("Buffer " + m_buffers[bid].name + " has no producer");
                }

                int producer = bufferProducerOf[bid];
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
                for (auto id : pass.reads)
                    std::cout << m_textures.at(id).name << " ";
                std::cout << "\n";
            }
            if (!pass.readBufs.empty())
            {
                std::cout << "  Buffer Reads:  ";
                for (auto id : pass.readBufs)
                    std::cout << m_buffers.at(id).name << " ";
                std::cout << "\n";
            }
            if (!pass.writes.empty())
            {
                std::cout << "  Texture Writes:  ";
                for (auto id : pass.writes)
                    std::cout << m_textures.at(id).name << " ";
                std::cout << "\n";
            }
            if (!pass.writeBufs.empty())
            {
                std::cout << "  Buffer Writes:  ";
                for (auto id : pass.writeBufs)
                    std::cout << m_buffers.at(id).name << " ";
                std::cout << "\n";
            }
        }
        std::cout << "\n";
    }

    void RenderGraph::allocateTextures(std::shared_ptr<rhi::RHIDevice> device, uint32_t frameWidth, uint32_t frameHeight)
    {

        deleteTextures(device);

        rhi::RHICommandBuffer *cmd = device->createCommandBuffer();
        for (auto &[tid, desc] : m_textures)
        {
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

            if (!m_allocatedTextures.count(tid))
            {
                std::cerr << "[RenderGraph] allocating texture '" << desc.name << "' " << textureDesc.size.width << "x" << textureDesc.size.height << "\n";
                m_allocatedTextures[tid] = device->createTexture(textureDesc);
                rhi::TextureBarrier textureBarrier;
                textureBarrier.texture = m_allocatedTextures[tid];
                textureBarrier.before = rhi::ResourceState::Undefined;
                textureBarrier.after = rhi::ResourceState::ShaderRead;
                cmd->textureBarrier(textureBarrier);
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
            std::cerr << "[RenderGraph] reallocating texture '" << desc.name << "' " << textureDesc.size.width << "x" << textureDesc.size.height << "\n";
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
                if (tid == id)
                {
                    return true;
                }
            }
            for (auto &bid : pass.writeBufs)
            {
                if (bid == id)
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
    void RenderGraph::execute(rhi::RHICommandBuffer *cmd, const RenderContext &ctx, RendererSettings &settings, const RGResources &resources)
    {

        for (auto &idx : m_executionOrder)
        {
            m_passes[idx].execute(cmd, resources, ctx, settings);
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
            for (auto &tid : pass.writes)
            {
                writerOf[tid] = pass.name;
            }
        }
        for (auto &pass : m_passes)
        {
            for (auto &tid : pass.reads)
            {
                if (!writerOf.count(tid))
                {
                    errors.push_back({pass.name,
                                      m_textures.at(tid).name,
                                      "reads texture with no writer"});
                }
            }
        }

        std::unordered_map<RGTextureID, std::string> firstWriter;

        for (int idx : m_executionOrder)
        {
            auto &pass = m_passes[idx];
            for (auto &tid : pass.writes)
            {
                if (!m_textures.at(tid).transient)
                    continue;

                if (firstWriter.count(tid))
                {
                    errors.push_back(
                        {pass.name,
                         m_textures.at(tid).name,
                         "write-after-write: also written by " + firstWriter.at(tid)});
                }
                firstWriter[tid] = pass.name;
            }
        };

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
                ImGui::Text("%s", m_textures.at(pass.reads[i]).name.c_str());
                ImNodes::EndInputAttribute();

                readersOf[pass.reads[i]].push_back(passIdx);
            }

            for (int i = 0; i < (int)pass.writes.size(); i++)
            {
                int pinId = passIdx * 100 + 50 + i;
                ImNodes::BeginOutputAttribute(pinId);
                ImGui::Text("%s", m_textures.at(pass.writes[i]).name.c_str());
                ImNodes::EndOutputAttribute();

                writerOf[pass.writes[i]] = passIdx;
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
            if (writes[i] == tid)
                return i;
        return -1;
    }

    int RenderGraph::readerInputIndex(int passIdx, RGTextureID tid)
    {
        auto &reads = m_passes[passIdx].reads;
        for (int i = 0; i < (int)reads.size(); i++)
            if (reads[i] == tid)
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
                lifetime[tid].createdAt = i;
                lifetime[tid].lastUsedAt = i;
            };
            for (auto &tid : pass.reads)
            {

                lifetime[tid].lastUsedAt = std::max(lifetime[tid].lastUsedAt, i);
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
        if (ImGui::BeginTable("lifetimes_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Resource");
            ImGui::TableSetupColumn("Created at");
            ImGui::TableSetupColumn("Last used at");
            ImGui::TableSetupColumn("Span");
            ImGui::TableHeadersRow();

            for (auto tid : ids)
            {
                auto &lt = lifetimes[tid];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(m_textures.at(tid).name.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(m_passes[m_executionOrder[lt.createdAt]].name.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(m_passes[m_executionOrder[lt.lastUsedAt]].name.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%d", lt.lastUsedAt - lt.createdAt + 1);
            }
            ImGui::EndTable();
        }
        ImGui::End();
    }
} // namespace nitro::renderer
