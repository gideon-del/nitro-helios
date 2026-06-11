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

        float gBufferMode;
        float pad[3];
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
        DeferredLightingPass(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<rhi::RHISwapchain> swapchain, std::vector<rhi::RHITexture *> &cascades, GBuffer &gBuffer, std::string shaderDir, bool isMetal);
        ~DeferredLightingPass();
        void execute(rhi::RHICommandBuffer *cmd, DeferredLightingFrameData frameData);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::shared_ptr<rhi::RHISwapchain> m_swapchain;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_uniformBufferDescriptorLayout;
        rhi::RHIDescriptorLayout *m_gBufferDescriptorLayout;
        rhi::RHIDescriptorLayout *m_shadowDescriptorLayout;
        PerFrame<DeferredLightingResource> m_resources;
        bool m_isMetal;
    };
} // namespace nitro::renderer
