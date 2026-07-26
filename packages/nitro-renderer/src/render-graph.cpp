#include <nitro-renderer/render-graph.h>

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
} // namespace nitro::renderer
