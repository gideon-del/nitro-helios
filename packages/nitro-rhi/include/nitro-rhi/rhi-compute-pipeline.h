#pragma once
#include "rhi-pipeline.h"

namespace nitro::rhi
{
    struct ComputePipelineDesc
    {
        ShaderDesc shader;
        std::vector<RHIDescriptorLayout *> layouts;
        bool hasPushConstant = false;
        uint32_t pushConstantSize = 0;
        uint32_t threadGroupSizeX = 16;
        uint32_t threadGroupSizeY = 16;
        uint32_t threadGroupSizeZ = 1;
    };

    class RHIComputePipeline
    {
    public:
        virtual ~RHIComputePipeline() = default;
    };
} // namespace nitro::rhi
