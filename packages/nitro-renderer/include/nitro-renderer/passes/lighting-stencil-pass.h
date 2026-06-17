#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/passes/geometry-pass.h>
#include <nitro-renderer/per-frame.h>
#include <nitro-renderer/settings.h>

namespace nitro::renderer
{
    struct LightingStencilResource
    {
        rhi::RHIBuffer *uniformBuffer;
        rhi::RHIDescriptorSet *descriptorSet;
    };
    struct LightStencilCamera
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 invViewProj;
    };

    struct LightStencilPushConstant
    {
        glm::mat4 model;
        glm::vec4 lightPosition;
        glm::vec4 lightColor;
        glm::vec2 screenSize;
        float radius;
        float intensity;
    };

    class LightingStencilPass
    {
    public:
        LightingStencilPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, GBuffer &gBuffer,
                            std::string shaderDir,
                            bool isMetal);
        ~LightingStencilPass();
        void execute(rhi::RHICommandBuffer *cmd, LightingSettings &settings, LightStencilCamera camera);
        void resize(uint32_t width, uint32_t height, GBuffer &gBuffer);
        rhi::RHITexture *getLightingTexture() { return m_lightingTexture; }

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHITexture *m_lightingTexture;
        rhi::RHIDescriptorLayout *m_descriptorLayout;

        rhi::RHIRenderPass *m_stencilPass;
        rhi::RHIRenderPass *m_lightVolumePass;

        rhi::RHIPipeline *m_stencilPipeline;
        rhi::RHIPipeline *m_lightVolumePipeline;

        uint32_t m_frameWidth;
        uint32_t m_frameHeight;
        PerFrame<LightingStencilResource> m_resources;
    };
} // namespace nitro::renderer
