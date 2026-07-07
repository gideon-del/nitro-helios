#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/per-frame.h>

namespace nitro::renderer
{
    struct ToneMapPassUBO
    {
        float exposure;
        uint mode;
        float _pad[2];
    };

    class ToneMapPass
    {
    public:
        ToneMapPass(std::shared_ptr<rhi::RHIDevice> device,
                    rhi::RHITexture *hdrTexture,
                    uint32_t width,
                    uint32_t height,
                    std::string shaderDir,
                    bool isMetal);

        ~ToneMapPass();
        void resize(rhi::RHITexture *hdrTexture,
                    uint32_t width,
                    uint32_t height);
        void execute(rhi::RHICommandBuffer *cmd, ToneMapPassUBO ubo);

        rhi::RHITexture *getToneMappedTexture() { return m_toneMappedTexture; }

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIRenderPass *m_renderPass;
        rhi::RHITexture *m_toneMappedTexture;
        rhi::RHIDescriptorSet *m_descriptorSet;
        uint32_t m_width, m_height;
    };
} // namespace nitro::renderer
