#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/per-frame.h>
#include <nitro-renderer/render-graph.h>
#include <glm/glm.hpp>

namespace nitro::renderer
{
    struct SkyboxPassResource
    {
        rhi::RHIBuffer *uniformBuffer;
        rhi::RHIDescriptorSet *descriptorSet;
    };
    struct SkyboxPassUBO
    {
        glm::mat4 viewProj;
        glm::vec2 screenSize;
        float pads[2];
    };
    struct SkyboxTextures
    {
        rhi::RHITexture *cubemapTexture;
        RGTextureID skybox;
    };
    class SkyboxPass
    {
    public:
        SkyboxPass(std::shared_ptr<rhi::RHIDevice> device,
                   uint32_t width,
                   uint32_t height,
                   std::string shaderDir,
                   bool isMetal);
        ~SkyboxPass();
        void resize(uint32_t width, uint32_t heigh);
        void execute(rhi::RHICommandBuffer *cmd, SkyboxPassUBO ubo);

        rhi::RHITexture *getSkyboxTexture() { return m_skyboxTexture; }

        void bindResources(const RGResources &resources, const SkyboxTextures &textures);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::uint32_t m_width, m_height;
        rhi::RHITexture *m_skyboxTexture;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIRenderPass *m_renderPass;
        PerFrame<SkyboxPassResource> m_resources;
    };

} // namespace nitro::renderer
