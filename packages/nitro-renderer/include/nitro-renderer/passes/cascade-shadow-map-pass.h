#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/scene.h>
#include <nitro-renderer/per-frame.h>
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

    struct CascadeShadowMapResource
    {
        rhi::RHIBuffer *uniformBuffer;
        rhi::RHIDescriptorSet *descriptorSet;
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
        PerFrame<CascadeShadowMapResource> m_resources;
    };
} // namespace nitro::renderer
