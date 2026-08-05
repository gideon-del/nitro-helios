#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>
#include <nitro-renderer/render-graph.h>

namespace nitro::renderer
{

    struct EmitterDesc
    {
        glm::vec4 position;
        glm::vec4 direction;
        glm::vec4 startColor;
        glm::vec4 endColor;
        float spawnRate;
        float initialSpeed;
        float speedVariance;
        float spread;
        float minLifetime;
        float maxLifetime;
        float spawnAccumulator = 0.0f;
    };

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
        static constexpr uint32_t s_MAX_EMITTERS = 100000;

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIComputePipeline *m_computePipeline;
        rhi::RHIDescriptorSet *m_descriptorSet;
        bool m_hasUploadedEmitters = false;
    };
} // namespace nitro::renderer
