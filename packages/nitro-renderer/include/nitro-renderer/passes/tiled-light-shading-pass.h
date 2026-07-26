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
        rhi::RHIBuffer *uniformBuffer;
        rhi::RHIDescriptorSet *descriptorSet;
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

    struct TileLightShadingTextureIDs
    {
        RGTextureID gDepth;
        RGTextureID gNormal;
        RGTextureID tileLightTex;
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
        void bindResources(const RGResources &resources, const TileLightShadingTextureIDs textures, const PerFrame<TileLightingComputeResource> &tileResources);

        void execute(rhi::RHICommandBuffer *cmd, TiledLightPassUBO ubo);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHITexture *m_lightTexture;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIRenderPass *m_renderPass;
        PerFrame<TiledLightPassResource> m_resources;
        uint32_t m_width, m_height;
    };
} // namespace nitro::renderer
