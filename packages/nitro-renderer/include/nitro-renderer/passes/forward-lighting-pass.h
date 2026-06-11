#pragma once

#include <nitro-renderer/frame-resource.h>
#include <nitro-renderer/per-frame.h>
#include <nitro-renderer/scene.h>
#include <glm/glm.hpp>
namespace nitro::renderer
{

    struct FrameData
    {
        glm::mat4 view;
        glm::mat4 proj;

        glm::vec4 cameraPos;

        glm::vec4 lightPos;
        glm::vec4 lightColor = glm::vec4(1.0f);

        glm::mat4 lightViewProj[4];
        glm::vec4 cascadeSplit;

        float ambient = 0.3f;
        float Ka = 1.0f;
        float Kd = 0.8f;
        float Ks = 0.9f;
        float shininess = 32.0f;

        float shadowBias;
        float shadowNormalBias;
        float showCascadeColors;
        float padding;
    };

    struct ForwardLightingResource
    {
        rhi::RHIBuffer *uniformBuffer;
        rhi::RHIDescriptorSet *mainDescriptorSet;
        rhi::RHIDescriptorSet *shadowDescriptorSet;
    };
    class ForwardLightingPass
    {
    public:
        ForwardLightingPass(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<rhi::RHISwapchain> swapchain, std::vector<rhi::RHITexture *> &cascades, std::string shaderDir, bool isMetal);
        ~ForwardLightingPass();
        void execute(rhi::RHICommandBuffer *cmd, Scene &scene, FrameData frameData);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::shared_ptr<rhi::RHISwapchain> m_swapchain;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_mainDescriptorLayout;
        rhi::RHIDescriptorLayout *m_shadowDescriptorLayout;
        PerFrame<ForwardLightingResource> m_resources;
    };
} // namespace nitro::renderer
