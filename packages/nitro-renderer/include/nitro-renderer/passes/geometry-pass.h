#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/scene.h>
#include <nitro-renderer/per-frame.h>
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

    struct GeometryPassResource
    {
        rhi::RHIBuffer *uniformBuffer;
        rhi::RHIDescriptorSet *descriptorSet;
    };

    struct GeometryCameraBuffer
    {
        glm::mat4 view;
        glm::mat4 proj;
    };

    class GeometryPass
    {
    public:
        GeometryPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, rhi::RHITexture *depthTexture, std::string shaderDir, bool isMetal = false);

        ~GeometryPass();
        void execute(rhi::RHICommandBuffer *cmd, GeometryCameraBuffer geometryCamera, Scene &scene);
        void resize(uint32_t width, uint32_t height, rhi::RHITexture *depthTexture);
        GBuffer gBuffer;

    private:
        uint32_t m_width;
        uint32_t m_height;
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIRenderPass *m_renderPass;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        PerFrame<GeometryPassResource> m_resources;
    };
}