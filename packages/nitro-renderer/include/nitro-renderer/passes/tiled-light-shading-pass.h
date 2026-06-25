#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/passes/geometry-pass.h>
#include <nitro-renderer/passes/tiled-deffered-compute-pass.h>
#include <nitro-renderer/per-frame.h>
#include <nitro-renderer/settings.h>
#include <glm/glm.hpp>

namespace nitro::renderer
{
    struct TiledLightPassResource
    {
        rhi::RHIBuffer *uniformBuffer;
        rhi::RHIDescriptorSet *descriptorSet;
    };

    struct TiledLightPassUBO
    {
        glm::mat4 invViewProj;
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
                             GBuffer &gBuffer,
                             const PerFrame<TileLightingComputeResource> &tiledResources,
                             std::string shaderDir,
                             bool isMetal);
        ~TileLightShadingPass();
        void resize(uint32_t width, uint32_t height, const GBuffer &gBuffer, const PerFrame<TileLightingComputeResource> &tileResources);
        rhi::RHITexture *getLightTexture() { return m_lightTexture; };

        void execute(rhi::RHICommandBuffer *cmd, TiledLightPassUBO ubo);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHITexture *m_lightTexture;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIRenderPass *m_renderPass;
        PerFrame<TiledLightPassResource> m_resources;
        uint32_t m_width, m_height;

        void m_linkDescriptorSet(TiledLightPassResource &resource, const GBuffer &gBuffer, const TileLightingComputeResource &tileResource);
        void m_createLightTextureAndRenderPass();
    };
} // namespace nitro::renderer
