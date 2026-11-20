#pragma once
#include "rhi-pipeline.h"
#include "nitro-core/utils.h"
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

        bool operator==(const ComputePipelineDesc &) const = default;
    };

    struct ComputePipelineDescHash
    {
        size_t operator()(const ComputePipelineDesc &desc) const
        {
            size_t seed = 0;

            hashCombine(seed, std::hash<int>{}(static_cast<int>(desc.shader.stage)));
            hashCombine(seed, std::hash<std::string>{}(desc.shader.filePath));
            hashCombine(seed, std::hash<std::string>{}(desc.shader.name));

            hashCombine(seed, desc.layouts.size());

            for (auto layout : desc.layouts)
            {
                hashCombine(seed, std::hash<RHIDescriptorLayout *>{}(layout));
            }

            hashCombine(seed, std::hash<bool>{}(desc.hasPushConstant));
            hashCombine(seed, std::hash<uint32_t>{}(desc.pushConstantSize));
            hashCombine(seed, std::hash<uint32_t>{}(desc.threadGroupSizeX));
            hashCombine(seed, std::hash<uint32_t>{}(desc.threadGroupSizeY));
            hashCombine(seed, std::hash<uint32_t>{}(desc.threadGroupSizeZ));

            return seed;
        };
    };

    class RHIComputePipeline
    {
    public:
        virtual ~RHIComputePipeline() = default;
    };
} // namespace nitro::rhi
