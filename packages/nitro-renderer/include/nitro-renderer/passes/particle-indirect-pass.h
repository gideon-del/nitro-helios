#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/render-graph.h>

namespace nitro::renderer
{
    struct ParticleIndirectResourceIDs
    {
        RGBufferID aliveCount;
        RGBufferID indirectDraw;
        RGBufferID indirectDispatch;
    };
    class ParticleIndirectPass
    {
    public:
        ParticleIndirectPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal);
        ~ParticleIndirectPass();
        void bindResources(const RGResources &resources, const ParticleIndirectResourceIDs ids);
        void execute(rhi::RHICommandBuffer *cmd);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIComputePipeline *m_computePipeline;
        rhi::RHIDescriptorSet *m_descriptorSet;
    };
} // namespace nitro::renderer
