#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/passes/geometry-pass.h>
#include <nitro-renderer/per-frame.h>
#include <nitro-renderer/settings.h>
#include <glm/glm.hpp>

namespace nitro::renderer
{

    struct TiledCameraUBO
    {
        glm::mat4 view;
        glm::mat4 invProj;
        glm::vec2 screenSize;
        float nearPlane;
        float farPlane;
        uint numTilesX;
        uint numTilesY;
        uint totalLightCount;
        float _pad;
    };
    struct TileDebug
    {
        uint lightCount;
        float minDepth;
        float maxDepth;
        float tileNear;
        float tileFar;
        uint overflow;
    };
    struct TileLightingComputeResource
    {
        rhi::RHIBuffer *tileLightCountBuffer;
        rhi::RHIBuffer *tileLightIndicesBuffer;
        rhi::RHIBuffer *tileLightDebugBuffer;
        rhi::RHIBuffer *pointLightBuffer;
        rhi::RHIBuffer *cameraUniformBuffer;
        rhi::RHIDescriptorSet *descriptorSet;
    };

    class TiledLightingComputePass
    {
    public:
        TiledLightingComputePass(std::shared_ptr<rhi::RHIDevice> device,
                                 uint32_t width,
                                 uint32_t height,
                                 uint32_t maxPointLights,
                                 GBuffer &gBuffer,
                                 std::string shader,
                                 bool isMetal);
        ~TiledLightingComputePass();
        void resize(uint32_t width, uint32_t height, GBuffer &gBuffer);
        void execute(rhi::RHICommandBuffer *cmd, LightingSettings &settings, TiledCameraUBO cameraUBO);
        PerFrame<TileLightingComputeResource> &getFrameResources() { return m_resources; };
        static constexpr uint32_t c_MAX_LIGHT_PER_TILE = 256;
        static constexpr uint32_t c_TILE_GROUP_SIZE = 16;

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIComputePipeline *m_computePipeline;

        PerFrame<TileLightingComputeResource> m_resources;
        uint32_t m_width, m_height, m_tileSizeX, m_tileSizeY, m_maxPointLights;
        void m_destroyBuffers();
        void m_createBuffers(TileLightingComputeResource &resource);
    };
} // namespace nitro::renderer
