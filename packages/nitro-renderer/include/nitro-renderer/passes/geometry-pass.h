#pragma once
#include <nitro-rhi/rhi-render-pass.h>
#include <nitro-rhi/rhi-texture.h>
#include <nitro-rhi/rhi-device.h>
#include <nitro-rhi/rhi-device.h>
#include <nitro-rhi/rhi-descriptor-layout.h>
#include <nitro-rhi/rhi-command-buffer.h>
#include <nitro-renderer/scene.h>
#include <nitro-renderer/frame-resource.h>
#include <glm/glm.hpp>
namespace nitro::renderer
{
    struct GBuffer
    {
        rhi::RHITexture *albedo;
        rhi::RHITexture *normal;
        rhi::RHITexture *material;
        rhi::RHITexture *emissive;

        rhi::RHITexture *depth;
    };

    struct GeometryCameraBuffer
    {
        glm::mat4 view;
        glm::mat4 proj;
    };

    class GeometryPass
    {
    public:
        GeometryPass(rhi::RHIDevice *device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal = false);

        ~GeometryPass();
        void execute(rhi::RHICommandBuffer *cmd, GeometryCameraBuffer geometryCamera, Scene &scene);

        GBuffer gBuffer;
        uint32_t width;
        uint32_t height;

    private:
        rhi::RHIDevice *m_device;
        rhi::RHIRenderPass *m_renderPass;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        std::vector<FrameResource> m_frameResources;
    };
}