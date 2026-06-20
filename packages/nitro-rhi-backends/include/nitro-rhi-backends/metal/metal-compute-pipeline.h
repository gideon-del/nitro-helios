#pragma once
#include <nitro-rhi/rhi-compute-pipeline.h>
#include <Metal/Metal.hpp>

namespace nitro::rhi::metal
{
    class MetalDevice;
    class MetalComputePipeline : public RHIComputePipeline
    {
    public:
        MetalComputePipeline(MetalDevice *device, const ComputePipelineDesc &desc);
        ~MetalComputePipeline() override;
        MTL::ComputePipelineState *pipelineState;
        uint32_t threadGroupSizeX = 16;
        uint32_t threadGroupSizeY = 16;
        uint32_t threadGroupSizeZ = 1;

    private:
        MetalDevice *m_device;
    };
} // namespace nitro::rhi::metal
