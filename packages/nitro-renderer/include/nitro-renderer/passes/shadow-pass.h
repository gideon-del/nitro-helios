#pragma once
#include <nitro-rhi/rhi-render-pass.h>
#include <nitro-rhi/rhi-texture.h>
#include <nitro-rhi/rhi-device.h>
#include <nitro-rhi/rhi-device.h>
#include <nitro-rhi/rhi-command-buffer.h>
#include <nitro-renderer/scene.h>
#include <glm/glm.hpp>

namespace nitro::renderer
{

    struct ShadowPushConstant
    {
        glm::mat4 model;
        int cascadeIndex;
        float pad[3];
    };

    class ShadowPass
    {
    public:
        ShadowPass(rhi::RHIDevice *device, int cascadeIndex);
        rhi::RHITexture *shadowTexture;
        int cascadeIndex;

        void execute(rhi::RHICommandBuffer *cmd, rhi::RHIPipeline *pipeline, rhi::RHIDescriptorSet *descriptorSet, Scene &scene);
        static float s_getUniformSplit(
            float near,
            float far,
            uint32_t cascadeCount,
            uint32_t cascadeIndex);
        static float s_getLogarithmSplit(
            float nearPlane,
            float farPlane,
            uint32_t cascadeCount,
            uint32_t cascadeIndex);
        static float s_getPracticalSplit(
            float nearPlane,
            float farPlane,
            uint32_t cascadeCount,
            uint32_t cascadeIndex, float lambda = 0.75f);
        static glm::mat4 s_calculateLightOrthoProj(
            float nearPlane,
            float farPlane,
            uint32_t cascadeCount,
            uint32_t cascadeIndex,
            float fov,
            float aspect,
            glm::mat4 cameraView,
            glm::mat4 lightView, float lambda = 0.75f);
        static constexpr uint32_t c_ShadowResolution = 2048;

    private:
        rhi::RHIRenderPass *m_renderPass;
    };
} // namespace nitro::renderer
