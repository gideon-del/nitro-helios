#pragma once
#include <nitro-rhi/rhi-render-pass.h>
#include <nitro-rhi/rhi-device.h>
#include <nitro-rhi/rhi-texture.h>
#include <nitro-rhi/rhi-descriptor-layout.h>
#include <nitro-renderer/frame-resource.h>
#include <nitro-renderer/scene.h>
#include <glm/glm.hpp>
#include "shadow-pass.h"

namespace nitro::renderer
{
    struct LightView
    {
        glm::mat4 lightViewProj[4];
    };

    struct CascadeShadowContext
    {
        glm::mat4 cameraView;
        glm::mat4 lightView;
        float cameraFar;
        float cameraNear;
        float fov;
        float aspect;
        float lambda = 0.75f;
    };
    class CascadeShadowMapPass
    {
    public:
        CascadeShadowMapPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal);

        ~CascadeShadowMapPass();

        void execute(rhi::RHICommandBuffer *cmd, Scene &scene, CascadeShadowContext ctx);
        std::vector<rhi::RHITexture *> &getCascadeTextures() { return m_cascades; };
        glm::mat4 lightViewProj[4];
        glm::vec4 cascadeSplit;
        static constexpr uint32_t CASCADE_COUNT = 4;

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::vector<rhi::RHITexture *> m_cascades;
        std::vector<ShadowPass> m_shadowPasses;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        std::vector<FrameResource> m_frameResources;
    };
} // namespace nitro::renderer
