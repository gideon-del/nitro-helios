#pragma once
#include "geometry-pass.h"
#include <nitro-rhi/rhi-render-pass.h>
#include <nitro-rhi/rhi-texture.h>
#include <nitro-rhi/rhi-device.h>
#include <nitro-rhi/rhi-device.h>
#include <nitro-rhi/rhi-descriptor-layout.h>
#include <nitro-rhi/rhi-command-buffer.h>
#include <nitro-renderer/scene.h>
#include <nitro-renderer/per-frame.h>
#include <glm/glm.hpp>
#include <nitro-renderer/settings.h>
namespace nitro::renderer
{
    struct DeferredLightingFrameData
    {

        glm::vec4 cameraPosition;
        glm::vec4 lightPosition;
        glm::vec4 lightColor;

        glm::mat4 invViewProj;
        glm::mat4 view;
        glm::mat4 lightViewProj[4];
        glm::vec4 cascadeSplit;

        float ambient;
        float Ka;
        float Kd;
        float Ks;
        float shininess;

        float shadowBias;
        float shadowNormalBias;
        float showCascadeColors;
        float debugMode;
        float lightMode;
        float roughness;
        float pad;
        PointLight pointLights[1000];
    };

    struct DeferredLightingResource
    {
        rhi::RHIBuffer *uniformBuffer;
        rhi::RHIDescriptorSet *mainDescriptorSet;
        rhi::RHIDescriptorSet *gBufferDescriptorSet;
        rhi::RHIDescriptorSet *shadowDescriptorSet;
    };

    class DeferredLightingPass
    {
    public:
        DeferredLightingPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::vector<rhi::RHITexture *> &cascades, GBuffer &gBuffer, rhi::RHITexture *cubeTexture, rhi::RHITexture *lightTexture, std::string shaderDir, bool isMetal);
        ~DeferredLightingPass();
        void execute(rhi::RHICommandBuffer *cmd, DeferredLightingFrameData frameData);
        void recreate(uint32_t width, uint32_t height, GBuffer &gBuffer, rhi::RHITexture *cubeTexture, rhi::RHITexture *lightTexture);
        rhi::RHITexture *getLightTexture() { return m_lightTexture; }

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        uint32_t m_width, m_height;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_uniformBufferDescriptorLayout;
        rhi::RHIDescriptorLayout *m_gBufferDescriptorLayout;
        rhi::RHIDescriptorLayout *m_shadowDescriptorLayout;
        PerFrame<DeferredLightingResource> m_resources;
        rhi::RHITexture *m_lightTexture;
        rhi::RHIRenderPass *m_renderPass;
    };
} // namespace nitro::renderer
