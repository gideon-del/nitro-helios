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

    struct ParticleUpdateResourceIDs
    {
        RGBufferID particleId;
        RGBufferID aliveIndicesId;
        RGBufferID aliveCounterId;
        RGBufferID deadListId;
    };

    class ParticleUpdatePass
    {
    public:
        ParticleUpdatePass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal);
        ~ParticleUpdatePass();
        void uploadDeadList(const RGResources &resources, RGBufferID deadList);
        void bindResources(const RGResources &resources, const ParticleUpdateResourceIDs ids);
        void execute(rhi::RHICommandBuffer *cmd, ParticlePushConstant pc, rhi::RHIBuffer *indirectDispatch);
        static constexpr uint32_t s_MAX_PARTICLE_COUNT = 1000000;

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIComputePipeline *m_computePipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIDescriptorSet *m_descriptorSet;
        bool m_uploadedParticles = false;
    };

} // namespace nitro::renderer
