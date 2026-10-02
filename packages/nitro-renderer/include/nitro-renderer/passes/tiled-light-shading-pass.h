#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/passes/geometry-pass.h>
#include <nitro-renderer/passes/tiled-deffered-compute-pass.h>
#include <nitro-renderer/per-frame.h>
#include <nitro-renderer/settings.h>
#include <nitro-renderer/render-graph.h>
#include <glm/glm.hpp>

namespace nitro::renderer
{
    struct TiledLightPassResource
    {
        rhi::RHIBuffer *lastTileLightCountBuffer = nullptr;
        rhi::RHIBuffer *lastTileLightIndicesBuffer = nullptr;
        rhi::RHIBuffer *lastTileLightDebugBuffer = nullptr;
        rhi::RHIBuffer *lastPointLightBuffer = nullptr;
        rhi::RHITexture *lastDepthTexture = nullptr;

        rhi::RHITexture *lastNormalTexture = nullptr;
        rhi::RHIBuffer *uniformBuffer;
        rhi::RHIDescriptorSet *descriptorSet;
    };

    struct TiledLightPassRGResource
    {
        RGBufferID tileLightCount;
        RGBufferID tileLightIndices;
        RGBufferID tileLightDebug;
        RGTextureID depthTexture;
        RGTextureID normalTexture;
        RGTextureID pointLightTexture;
    };

    struct TiledLightPassUBO
    {
        glm::mat4 invViewProj;
        glm::mat4 view;
        glm::vec2 screenSize;
        uint numTilesX;
        uint maxLightPerTile = 256;
        uint showHeatMap = 0;
        float pad[3];
    };

    class TileLightShadingPass
    {

    public:
        TileLightShadingPass(std::shared_ptr<rhi::RHIDevice> device,
                             uint32_t width,
                             uint32_t height,
                             std::string shaderDir,
                             bool isMetal);
        ~TileLightShadingPass();
        void resize(uint32_t width, uint32_t height);

        void execute(rhi::RHICommandBuffer *cmd, const RGResources &rgResources, const TiledLightPassRGResource &rgResourceIds, Scene &scene, TiledLightPassUBO ubo);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHITexture *m_lightTexture;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIRenderPass *m_renderPass = nullptr;
        PerFrame<TiledLightPassResource> m_resources;
        uint32_t m_width, m_height;
        rhi::RHITexture *m_lastPointLightTexture = nullptr;

        bool isDescriptorSetStale(const RGResources &rgResources, const TiledLightPassRGResource &rgResourceIds, TiledLightPassResource &resource, rhi::RHIBuffer *pointLightBuffer);
        void bindDescriptorSet(const RGResources &rgResources, const TiledLightPassRGResource &rgResourceIds, TiledLightPassResource &resource, rhi::RHIBuffer *pointLightBuffer);

        bool isRenderPassStale(const RGResources &rgResources, const TiledLightPassRGResource &rgResourceIds);
        void updateRenderPass(const RGResources &rgResources, const TiledLightPassRGResource &rgResourceIds);
    };
} // namespace nitro::renderer
