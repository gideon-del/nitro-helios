#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/scene.h>
#include <nitro-renderer/per-frame.h>
#include <nitro-renderer/settings.h>
#include <nitro-renderer/render-graph.h>
#include <glm/glm.hpp>
namespace nitro::renderer
{
    struct GBuffer
    {
        RGTextureID albedo;
        RGTextureID normal;
        RGTextureID material;
        RGTextureID emissive;

        RGTextureID depth;
    };

    struct GeometryPassResource
    {
        rhi::RHIBuffer *uniformBuffer;
        rhi::RHIDescriptorSet *descriptorSet;
        rhi::RHIBuffer *lastMeshInstanceBuffer = nullptr;
        rhi::RHIBuffer *lastMaterialBuffer = nullptr;
    };

    struct GeometryCameraBuffer
    {
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct DefaultTextures
    {
        rhi::RHITexture *baseColor;
        rhi::RHITexture *normal;
        rhi::RHITexture *metallicRoughness;
        rhi::RHITexture *ao;
        rhi::RHITexture *emissive;
    };

    class GeometryPass
    {
    public:
        GeometryPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal);

        ~GeometryPass();
        void execute(rhi::RHICommandBuffer *cmd, GeometryCameraBuffer geometryCamera, Scene &scene, LightingSettings &settings, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer);
        void resize(uint32_t width, uint32_t height);
        void bindResources(const RGResources &resources, const GBuffer &gBuffer);

    private:
        uint32_t m_width;
        uint32_t m_height;
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIRenderPass *m_renderPass = nullptr;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIDescriptorLayout *m_materialDescriptorLayout;
        PerFrame<GeometryPassResource> m_resources;

        bool isSceneBuffersStale(Scene &scene, GeometryPassResource &resource);
        void bindSceneBuffers(Scene &scene, GeometryPassResource &resource);
    };
}