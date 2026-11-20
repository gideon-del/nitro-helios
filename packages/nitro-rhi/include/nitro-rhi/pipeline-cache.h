#pragma once
#include "nitro-rhi/rhi-pipeline.h"
#include "nitro-rhi/rhi-compute-pipeline.h"
#include "nitro-rhi/rhi-device.h"

namespace nitro::rhi
{

    using PipelineRef = RHIPipeline *;
    using ComputePipelineRef = RHIComputePipeline *;

    inline auto byLocation = [](const auto &a, const auto &b)
    {
        return a.location < b.location;
    };
    inline auto byStage = [](const auto &a, const auto &b)
    {
        return static_cast<int>(a.stage) < static_cast<int>(b.stage);
    };
    struct PipelineKey
    {
        PipelineDesc desc;

        explicit PipelineKey(const PipelineDesc &in) : desc(in)
        {
            std::sort(desc.shaders.begin(), desc.shaders.end(), byStage);
            std::sort(desc.vertexLayout.attributes.begin(), desc.vertexLayout.attributes.end(), byLocation);
            desc.stencil.front.reference = 0;
            desc.stencil.back.reference = 0;
        }
        bool operator==(const PipelineKey &) const = default;
    };

    struct PipelineKeyHash
    {
        size_t operator()(const PipelineKey &key) const
        {
            return PipelineDescHash{}(key.desc);
        }
    };

    class PipelineCache
    {
    public:
        explicit PipelineCache(RHIDevice &device) : m_device(device) {};
        ~PipelineCache();
        PipelineRef get(const PipelineDesc &desc);
        ComputePipelineRef get(const ComputePipelineDesc &desc);

    private:
        RHIDevice &m_device;
        std::unordered_map<PipelineKey, PipelineRef, PipelineKeyHash> m_graphics;
        std::unordered_map<ComputePipelineDesc, ComputePipelineRef, ComputePipelineDescHash> m_compute;
    };
} // namespace nitro::rhi
