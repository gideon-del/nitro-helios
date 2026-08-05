#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/render-graph.h>

namespace nitro::renderer
{
    struct ParticleCompactResourceIDs
    {
        RGBufferID particleIds;
        RGBufferID aliveIndicesId;
        RGBufferID aliveCountId;
    };

    struct ParticleCompactPushConstant
    {
        uint particleCount;
    };

    class ParticleCompactPass
    {
    public:
        ParticleCompactPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal);
        ~ParticleCompactPass();
        void bindResources(const RGResources &resources, const ParticleCompactResourceIDs &ids);
        void execute(rhi::RHICommandBuffer *cmd, ParticleCompactPushConstant &pc);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIComputePipeline *m_computePipeline;
        rhi::RHIDescriptorSet *m_descriptorSet;
    };
} // namespace nitro::renderer
