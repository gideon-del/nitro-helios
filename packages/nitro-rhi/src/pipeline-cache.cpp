#include "nitro-rhi/pipeline-cache.h"

namespace nitro::rhi
{
    PipelineCache::~PipelineCache()
    {
        for (auto &[key, pipeline] : m_graphics)
            m_device.destroyPipeline(pipeline);
        for (auto &[key, pipeline] : m_compute)
            m_device.destroyComputePipeline(pipeline);
    }

    PipelineRef PipelineCache::get(const PipelineDesc &desc)
    {
        PipelineKey key{desc};

        auto it = m_graphics.find(key);

        if (it != m_graphics.end())
        {
            return it->second;
        }

        auto pipeline = m_device.createPipeline(key.desc);
        assert(pipeline && "pipeline creation failed");
        m_graphics.emplace(std::move(key), pipeline);

        return pipeline;
    }
    ComputePipelineRef PipelineCache::get(const ComputePipelineDesc &desc)
    {

        auto it = m_compute.find(desc);

        if (it != m_compute.end())
        {
            return it->second;
        }

        auto pipeline = m_device.createComputePipeline(desc);
        assert(pipeline && "pipeline creation failed");
        m_compute[desc] = pipeline;

        return pipeline;
    }
} // namespace nitro::rhi
