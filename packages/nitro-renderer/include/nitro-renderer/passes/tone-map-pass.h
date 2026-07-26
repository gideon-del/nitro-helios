#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/per-frame.h>
#include <nitro-renderer/render-graph.h>

namespace nitro::renderer
{
    struct ToneMapPassUBO
    {
        float exposure;
        uint mode;
        float _pad[2];
    };
    struct ToneMapPassTextures
    {
        rhi::RHITexture *hdrTexture;
        rhi::RHITexture *output;
    };

    class ToneMapPass
    {
    public:
        ToneMapPass(std::shared_ptr<rhi::RHIDevice> device,
                    uint32_t width,
                    uint32_t height,
                    std::string shaderDir,
                    bool isMetal);

        ~ToneMapPass();
        void resize(
            uint32_t width,
            uint32_t height);
        void bindResource(const RGResources &resource, const RGTextureID tonemapID);
        void execute(rhi::RHICommandBuffer *cmd, ToneMapPassUBO ubo, rhi::RHITexture *hdrTexture);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIRenderPass *m_renderPass = nullptr;
        rhi::RHIDescriptorSet *m_descriptorSet;
        uint32_t m_width, m_height;
    };
} // namespace nitro::renderer
