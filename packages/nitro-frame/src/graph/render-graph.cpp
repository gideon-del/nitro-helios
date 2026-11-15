#include "nitro-frame/graph/render-graph.h"
#include <sstream>
namespace nitro::frame
{

    static bool covers(rhi::ResourceState write, rhi::ResourceState read)
    {
        using S = rhi::ResourceState;
        return write == read || (write == S::ShaderWrite && read == S::ShaderRead) || (write == S::DepthWrite && read == S::DepthRead);
    }

    static std::vector<TextureAccess> mergeAccesses(const std::vector<TextureAccess> &in)
    {
        std::vector<TextureAccess> out;
        for (const auto &a : in)
        {
            auto it = std::find_if(out.begin(), out.end(), [&](const TextureAccess &b)
                                   { return b.handle == a.handle && b.history == a.history && b.range == a.range; });
            if (it == out.end())
            {
                out.push_back(a);
                continue;
            }

            if (covers(it->to, a.to))
            {
            }
            else if (covers(a.to, it->to))
            {
                it->to = a.to;
            }
            else
                assert(false && "pass needs one subresource in two incompatible states");
        }
        return out;
    }
    static std::vector<BufferAccess> mergeAccesses(const std::vector<BufferAccess> &in)
    {
        std::vector<BufferAccess> out;
        for (const auto &a : in)
        {
            auto it = std::find_if(out.begin(), out.end(), [&](const BufferAccess &b)
                                   { return b.handle == a.handle && b.history == a.history; });
            if (it == out.end())
            {
                out.push_back(a);
                continue;
            }

            if (covers(it->to, a.to))
            {
            }
            else if (covers(a.to, it->to))
            {
                it->to = a.to;
            }
            else
                assert(false && "pass needs one subresource in two incompatible states");
        }
        return out;
    }
    LogicalTextureHandle RenderGraphBuilder::addLogical(const PhysicalTextureHandle handle, bool history)
    {
        const auto &info = m_resourceManager.info(handle);
        LogicalKey lookup{info.key, history};
        auto it = m_logicalTextureLookup.find(lookup);

        if (it != m_logicalTextureLookup.end())
        {
            return it->second;
        }

        LogicalTexture logical{.name = info.key.name, .physicalHandle = handle, .history = history};
        logical.versions = {info.options.lifetime == ResourceLifetimeType::Transient
                                ? ResourceVersion{}
                                : ResourceVersion{.producer = kExternalProducer}};
        auto logicalHandle = m_logicalTextures.emplace(logical);
        m_logicalTextureLookup[lookup] = logicalHandle;
        return logicalHandle;
    }

    LogicalBufferHandle RenderGraphBuilder::addLogical(const PhysicalBufferHandle handle, bool history)
    {
        const auto &info = m_resourceManager.info(handle);
        LogicalKey lookup{info.key, history};

        auto it = m_logicalBufferLookup.find(lookup);

        if (it != m_logicalBufferLookup.end())
        {
            return it->second;
        }

        LogicalBuffer logical{.name = info.key.name, .physicalHandle = handle, .history = history};
        logical.versions = {ResourceVersion{.producer = kExternalProducer}};
        auto logicalHandle = m_logicalBuffers.emplace(logical);

        m_logicalBufferLookup[lookup] = logicalHandle;
        return logicalHandle;
    }

    LogicalTextureHandle RenderGraphBuilder::createTexture(const std::string name, const rhi::TextureDesc &desc, const ResourceSize &size, const GPUResourceOptions &options)
    {

        assert(options.instanceType != ResourceInstanceType::Temporal && "use createTemporalTexture() to create a temporal resource");

        auto handle = m_resourceManager.createTexture(name, desc, size, options);

        return addLogical(handle, false);
    };

    LogicalTextureHandle RenderGraphBuilder::createTexture(const PhysicalTextureHandle handle)
    {
        auto &info = m_resourceManager.info(handle);

        assert(info.options.instanceType != ResourceInstanceType::Temporal && "use createTemporalTexture() to create a temporal resource");
        return addLogical(handle, false);
    }

    LogicalBufferHandle RenderGraphBuilder::createBuffer(const std::string name, const rhi::BufferDesc &desc, const ResourceSize &size, const GPUResourceOptions &options)
    {

        assert(options.instanceType != ResourceInstanceType::Temporal && "use createTemporalBuffer() to create a temporal resource");

        auto handle = m_resourceManager.createBuffer(name, desc, size, options);

        return addLogical(handle, false);
    };
    LogicalBufferHandle RenderGraphBuilder::createBuffer(const PhysicalBufferHandle handle)
    {
        auto &info = m_resourceManager.info(handle);

        assert(info.options.instanceType != ResourceInstanceType::Temporal && "use createTemporalBuffer() to create a temporal resource");
        return addLogical(handle, false);
    }

    TemporalLogicalTexture RenderGraphBuilder::createTemporalTexture(const std::string name, const rhi::TextureDesc &desc, const ResourceSize &size, const GPUResourceOptions &options)
    {
        assert(options.instanceType == ResourceInstanceType::Temporal && "use createTexture() to create a other textures");

        auto handle = m_resourceManager.createTexture(name, desc, size, options);

        TemporalLogicalTexture temporal;
        temporal.current = addLogical(handle, false);
        temporal.previous = addLogical(handle, true);

        return temporal;
    }
    TemporalLogicalTexture RenderGraphBuilder::createTemporalTexture(const PhysicalTextureHandle handle)
    {
        auto &info = m_resourceManager.info(handle);
        assert(info.options.instanceType == ResourceInstanceType::Temporal && "use createTexture() to create a other textures");

        TemporalLogicalTexture temporal;
        temporal.current = addLogical(handle, false);
        temporal.previous = addLogical(handle, true);

        return temporal;
    }

    TemporalLogicalBuffer RenderGraphBuilder::createTemporalBuffer(const std::string name, const rhi::BufferDesc &desc, const ResourceSize &size, const GPUResourceOptions &options)
    {
        assert(options.instanceType == ResourceInstanceType::Temporal && "use createBuffer() to create a other buffers");

        auto handle = m_resourceManager.createBuffer(name, desc, size, options);

        TemporalLogicalBuffer temporal;
        temporal.current = addLogical(handle, false);
        temporal.previous = addLogical(handle, true);

        return temporal;
    }
    TemporalLogicalBuffer RenderGraphBuilder::createTemporalBuffer(const PhysicalBufferHandle handle)
    {
        auto &info = m_resourceManager.info(handle);
        assert(info.options.instanceType == ResourceInstanceType::Temporal && "use createBuffer() to create a other buffers");

        TemporalLogicalBuffer temporal;
        temporal.current = addLogical(handle, false);
        temporal.previous = addLogical(handle, true);

        return temporal;
    }

    LogicalTextureHandle RenderGraphBuilder::importTexture(const std::string &name, rhi::RHITextureHandle rhiHandle)
    {
        auto handle = m_resourceManager.importTexture(name, rhiHandle);

        return addLogical(handle, false);
    }

    LogicalBufferHandle RenderGraphBuilder::importBuffer(const std::string &name, rhi::RHIBufferHandle rhiHandle)
    {
        auto handle = m_resourceManager.importBuffer(name, rhiHandle);

        return addLogical(handle, false);
    }

    LogicalTexture *RenderGraphBuilder::get(const LogicalTextureHandle &handle)
    {
        return m_logicalTextures.get(handle);
    }
    LogicalBuffer *RenderGraphBuilder::get(const LogicalBufferHandle &handle)
    {
        return m_logicalBuffers.get(handle);
    }

    RenderPass &RenderGraphBuilder::addRenderPass(std::string name)
    {
        auto it = m_passNameToPassIdx.find(name);

        if (it != m_passNameToPassIdx.end())
        {
            return m_renderPasses[it->second];
        }
        auto index = static_cast<uint32_t>(m_renderPasses.size());

        m_renderPasses.push_back(RenderPass{index, name, *this});

        m_passNameToPassIdx[name] = index;
        return m_renderPasses[index];
    }

    const CompiledRenderGraph RenderGraphBuilder::compile()
    {
        m_graph = Graph();

        for (auto &pass : m_renderPasses)
        {
            m_graph.addNode(pass.index());
        }

        auto logicalTextureLoop = [&](const LogicalTextureHandle &handle, const LogicalTexture &logicalTexture)
        {
                                      std::optional<int> previousVersionIdx = std::nullopt;

                                      if (logicalTexture.history)
                                      {
                                          return;
                                      }
                                      for (int i = 0; i < logicalTexture.versions.size(); i++)
                                      {

                                          auto &currentVersion = logicalTexture.versions[i];

                                          bool hasProducer = currentVersion.producer.has_value();

                                          if (!hasProducer)
                                          {
                                              if (currentVersion.consumers.empty())
                                                  continue;
                                              throw std::runtime_error(std::format("{} does not have a producer", logicalTexture.name));
                                          }

                                          auto producerIdx = *currentVersion.producer;

                                         bool external = producerIdx == kExternalProducer;
               
                                         if(!external){
                                            for (auto &consumer : currentVersion.consumers)
                                         {
                                             if (consumer != producerIdx)
                                             {
                                                 m_graph.addEdge(producerIdx, consumer);
                                             }
                                         }
                                         }

                                          if (previousVersionIdx)
                                          {
                                              auto &previousVersion = logicalTexture.versions[*previousVersionIdx];
                                             if(previousVersion.producer &&*previousVersion.producer != kExternalProducer  && previousVersion.producer != producerIdx){
                                                 m_graph.addEdge(*previousVersion.producer, producerIdx);
                                             }

                                              for (auto &consumerIdx : previousVersion.consumers)
                                              {
                                                  if (consumerIdx != producerIdx)
                                                  {
                                                      m_graph.addEdge(consumerIdx, producerIdx);
                                                  }
                                              }
                                          }

                                          previousVersionIdx = i;
                                      } };
        m_logicalTextures.forEach(logicalTextureLoop);

        auto logicalBufferLoop = [&](const LogicalBufferHandle &handle, const LogicalBuffer &logicalBuffer)
        {
                                     std::optional<int> previousVersionIdx = std::nullopt;

                                     if (logicalBuffer.history)
                                     {
                                         return;
                                     }
                                     for (int i = 0; i < logicalBuffer.versions.size(); i++)
                                     {

                                         auto &currentVersion = logicalBuffer.versions[i];

                                         bool hasProducer = currentVersion.producer.has_value();

                                         if (!hasProducer)
                                         {
                                             if (currentVersion.consumers.empty())
                                                 continue;
                                             throw std::runtime_error(std::format("{} does not have a producer", logicalBuffer.name));
                                         }

                                         

                                         auto producerIdx = *currentVersion.producer;
                                         bool external = producerIdx == kExternalProducer;
               
                                         if(!external){
                                            for (auto &consumer : currentVersion.consumers)
                                         {
                                             if (consumer != producerIdx)
                                             {
                                                 m_graph.addEdge(producerIdx, consumer);
                                             }
                                         }
                                         }
                                         if (previousVersionIdx)
                                         {
                                             auto &previousVersion = logicalBuffer.versions[*previousVersionIdx];
 
                                             
                                             if(previousVersion.producer && *previousVersion.producer != kExternalProducer && previousVersion.producer != producerIdx){
                                                m_graph.addEdge(*previousVersion.producer, producerIdx);
                                             }
                                             

                                             for (auto &consumerIdx : previousVersion.consumers)
                                             {
                                                 if (consumerIdx != producerIdx)
                                                 {
                                                     m_graph.addEdge(consumerIdx, producerIdx);
                                                 }
                                             }
                                         }

                                         previousVersionIdx = i;
                                     } };
        m_logicalBuffers.forEach(logicalBufferLoop);
        m_executionOrder = m_graph.topoSort();
        auto cycles = m_graph.findCycles();

        if (!cycles.empty())
        {
            std::ostringstream ss;
            ss << "Cycle Found in render graph\n";
            for (auto &cycle : cycles)
            {
                for (size_t i = 0; i < cycle.size(); i++)
                {
                    ss << m_renderPasses[cycle[i]].name() << " ";

                    ss << (i == cycle.size() - 1 ? "." : "-> ");
                }

                ss << "\n";
            }

            throw std::runtime_error(ss.str());
        }
        m_lifetime = calculateLifetimes();
        CompiledRenderGraph compiledGraph;

        for (auto &passIdx : m_executionOrder)
        {
            compiledGraph.steps.push_back(buildRenderPassStep(passIdx));
        }

        return compiledGraph;
    }

    const CompiledRenderGraph::Step RenderGraphBuilder::buildRenderPassStep(int passIdx)
    {

        if (passIdx >= (int)m_renderPasses.size())
            throw std::runtime_error(std::format("No Render Pass with index: {}", passIdx));

        CompiledRenderGraph::Step renderPassStep;
        auto &renderPass = m_renderPasses[passIdx];

        renderPassStep.passIdx = passIdx;
        renderPassStep.textureBarriers = mergeAccesses(renderPass.getTextureAccess());
        renderPassStep.bufferBarriers = mergeAccesses(renderPass.getBufferAccess());

        return renderPassStep;
    };

    const PhysicalTextureLifetime RenderGraphBuilder::calculateLifetimes() const
    {

        PhysicalTextureLifetime lifetimes;
        std::unordered_map<uint32_t, uint32_t> passIdxToOrder;

        for (uint32_t i = 0; i < (uint32_t)m_executionOrder.size(); i++)
        {
            auto &passIdx = m_executionOrder[i];
            passIdxToOrder[passIdx] = i;
        }
        m_logicalTextures.forEach([&](const LogicalTextureHandle &handle, const LogicalTexture &logicalTexture)
                                  {

       auto& info = m_resourceManager.info(logicalTexture.physicalHandle);

       if(info.options.lifetime != ResourceLifetimeType::Transient)return;


            auto &firstVersion = logicalTexture.versions[0];

            if (!firstVersion.producer)
                return;

                assert(firstVersion.producer.value() > kExternalProducer && "Invalid Producer Index");

            auto &lifetime = lifetimes[logicalTexture.physicalHandle];
           

            lifetime.createdAt = passIdxToOrder[*firstVersion.producer];
            lifetime.lastUse = passIdxToOrder[*firstVersion.producer];

            for (auto &version : logicalTexture.versions)
            {

                lifetime.lastUse = std::max(lifetime.lastUse, (int)passIdxToOrder[*version.producer]);

                for (auto &consumer : version.consumers)
                {
                    lifetime.lastUse = std::max(lifetime.lastUse, (int)passIdxToOrder[consumer]);
                }
            } });

        return lifetimes;
    };
    uint32_t RenderGraphBuilder::orderOf(const std::string &passName) const
    {

        auto invalidPassIdx = std::numeric_limits<uint32_t>().max();
        if (!m_passNameToPassIdx.contains(passName))
            return invalidPassIdx;

        auto passIdx = m_passNameToPassIdx.at(passName);

        for (uint32_t i = 0; i < m_executionOrder.size(); i++)
        {
            if (m_executionOrder[i] == passIdx)
            {
                return i;
            }
        }

        return invalidPassIdx;
    }

    RenderPass &RenderPass::read(const LogicalTextureHandle &handle, const rhi::ResourceState &state, rhi::TextureSubresource range)
    {
        auto *logical = m_renderGraph.get(handle);
        assert(logical && "invalid or stale handle");

        logical->versions.back().consumers.push_back(m_index);
        m_textures.push_back({.handle = logical->physicalHandle,
                              .to = state,
                              .range = range,
                              .history = logical->history});

        return *this;
    }
    RenderPass &RenderPass::read(const LogicalBufferHandle &handle, const rhi::ResourceState &state)
    {
        auto *logical = m_renderGraph.get(handle);
        assert(logical && "invalid or stale handle");

        logical->versions.back().consumers.push_back(m_index);
        m_buffers.push_back({.handle = logical->physicalHandle,
                             .to = state,
                             .history = logical->history});

        return *this;
    }

    RenderPass &RenderPass::write(const LogicalTextureHandle &handle, const rhi::ResourceState &state, rhi::TextureSubresource range)
    {
        auto *logical = m_renderGraph.get(handle);
        assert(logical && "invalid or stale handle");
        assert(!logical->history && "can not write last frame resource");

        m_textures.push_back({.handle = logical->physicalHandle,
                              .to = state,
                              .range = range,
                              .history = false});
        if (logical->versions.back().producer == (int)m_index)
        {
            return *this;
        }
        auto &currentVersion = logical->versions.back();

        if (currentVersion.producer || !currentVersion.consumers.empty())
        {
            logical->versions.push_back({});
        }

        logical->versions.back().producer = m_index;

        return *this;
    }
    RenderPass &RenderPass::write(const LogicalBufferHandle &handle, const rhi::ResourceState &state)
    {
        auto *logical = m_renderGraph.get(handle);
        assert(logical && "invalid or stale handle");
        assert(!logical->history && "can not write last frame resource");

        m_buffers.push_back({.handle = logical->physicalHandle,
                             .to = state,
                             .history = false});
        auto &currentVersion = logical->versions.back();

        if (logical->versions.back().producer == (int)m_index)
        {
            return *this;
        }
        if (currentVersion.producer || !currentVersion.consumers.empty())
        {
            logical->versions.push_back({});
        }

        logical->versions.back().producer = m_index;
        return *this;
    }

    RenderPass &RenderPass::setExecuteFn(RenderPassExecute execute)
    {
        m_execute = execute;
        return *this;
    }

    size_t RenderGraphBuilder::structureHash() const
    {
        return 0;
    }

} // namespace nitro::frame
