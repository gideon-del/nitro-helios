#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>
#include <nitro-renderer/render-graph.h>
#include <nitro-renderer/particle-emitter-system.h>

namespace nitro::renderer
{

    struct ParticleEmitterPushConstant
    {
        uint emitterCount;
        uint frameIndex;
        uint maxParticles;
        float frameTime;
    };

    struct ParticleEmitterBufferIDs
    {
        RGBufferID particleID;
        RGBufferID deadListID;
        RGBufferID emitterID;
    };
    class ParticleEmitterPass
    {
    public:
        ParticleEmitterPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal);
        ~ParticleEmitterPass();
        void uploadInitialEmitter(const RGResources &resources, const RGBufferID emitterID);
        void bindResources(const RGResources &resources, const ParticleEmitterBufferIDs &ids);
        void execute(rhi::RHICommandBuffer *cmd, ParticleEmitterPushConstant pc);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIComputePipeline *m_computePipeline;
        rhi::RHIDescriptorSet *m_descriptorSet;
        bool m_hasUploadedEmitters = false;
    };
} // namespace nitro::renderer
