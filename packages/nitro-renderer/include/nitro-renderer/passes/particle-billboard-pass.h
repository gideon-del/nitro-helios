#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>
#include <nitro-renderer/render-graph.h>
#include <nitro-renderer/per-frame.h>

namespace nitro::renderer
{
    struct ParticleBillboardResourceIDs
    {
        RGBufferID particleId;
        RGBufferID aliveList;
        RGTextureID outputTexture;
    };

    struct ParticleBillboardUBO
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec4 up;
        glm::vec4 right;
    };

    struct ParticleBillBoardResource
    {
        rhi::RHIBuffer *uniformBuffer;
        rhi::RHIDescriptorSet *descriptorSet;
    };

    class ParticleBillboardPass
    {
    public:
        ParticleBillboardPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal);
        ~ParticleBillboardPass();
        void resize(uint32_t width, uint32_t height);
        void bindResources(const RGResources &resources, const ParticleBillboardResourceIDs particleResource);
        void execute(rhi::RHICommandBuffer *cmd, const ParticleBillboardUBO ubo, rhi::RHIBuffer *indirectDraw);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        uint32_t m_width, m_height;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIRenderPass *m_renderPass = nullptr;
        PerFrame<ParticleBillBoardResource> m_resources;
    };
} // namespace nitro::renderer
