#pragma once
#include <glm/glm.hpp>
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/render-graph.h>
namespace nitro::renderer
{

    struct ParticleDesc
    {
        glm::vec4 position;
        glm::vec4 velocity;
        glm::vec4 color;
        float lifetime;
        float age;
        float size;
        float _pads;
    };

    struct ParticlePushConstant
    {
        float dt;
        float gravity = 9.8;
        float drag = 1.0;
        uint particleCount;
    };

    class ParticleUpdatePass
    {
    public:
        ParticleUpdatePass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal);
        ~ParticleUpdatePass();
        void uploadInitalParticles(const RGResources &resources, RGBufferID particleId);
        void execute(rhi::RHICommandBuffer *cmd, ParticlePushConstant pc, const RGResources &resources, const RGBufferID particleId);
        static constexpr uint32_t s_MAX_PARTICLE_COUNT = 1000000;

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIComputePipeline *m_computePipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIDescriptorSet *m_descriptorSet;
        bool m_uploadedParticles = false;
    };

} // namespace nitro::renderer
