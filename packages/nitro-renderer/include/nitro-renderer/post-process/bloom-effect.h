#pragma once
#include <nitro-renderer/passes/render-passes.h>
#include <nitro-rhi/rhi.h>

namespace nitro::renderer
{
    class BloomEffect
    {
    public:
        BloomEffect(std::shared_ptr<rhi::RHIDevice> device,
                    uint32_t width, uint32_t height,
                    std::string shaderDir, bool isMetal);

        void resize(uint32_t width, uint32_t height);
        rhi::RHITexture *execute(rhi::RHICommandBuffer *cmd,
                                 rhi::RHITexture *inputTexture,
                                 const BloomSettings &settings);

    private:
        uint32_t m_width, m_height;
        std::unique_ptr<BrightnessPass> m_brightnessPass;
        std::unique_ptr<GaussianBlurPass> m_gaussianBlurPass;
        std::unique_ptr<CombineTexturePass> m_compositePass;
    };
} // namespace nitro::renderer
