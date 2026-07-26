#include <nitro-renderer/post-process/bloom-effect.h>

namespace nitro::renderer
{
    BloomEffect::BloomEffect(std::shared_ptr<rhi::RHIDevice> device,
                             uint32_t width, uint32_t height,
                             std::string shaderDir, bool isMetal) : m_width(width),
                                                                    m_height(height)

    {
        m_brightnessPass = std::make_unique<BrightnessPass>(
            device,
            m_width,
            m_height,
            shaderDir,
            isMetal);
        m_gaussianBlurPass = std::make_unique<GaussianBlurPass>(
            device,
            m_width,
            m_height,
            shaderDir,
            isMetal);
        m_compositePass = std::make_unique<CombineTexturePass>(
            device,
            m_width,
            m_height,
            shaderDir,
            isMetal);
    };

    void BloomEffect::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;

        m_brightnessPass->resize(m_width, m_height);
        m_gaussianBlurPass->resize(m_width, m_height);
        m_compositePass->resize(m_width, m_height);
    }

    void BloomEffect::execute(rhi::RHICommandBuffer *cmd,
                              BloomTextures textures, const BloomSettings &settings)
    {
        BrightnessPassPushConstant brightnessPc;

        brightnessPc.screenSize = glm::vec2(float(m_width), float(m_height));
        brightnessPc.threshold = settings.threshold;
        rhi::RHITexture *brightTexture = m_brightnessPass->execute(cmd, brightnessPc, textures.hdrTexture);

        GaussianBlurPushConstant gaussianPc;

        gaussianPc.inputTextureSize = brightnessPc.screenSize;
        gaussianPc.outputTextureSize = brightnessPc.screenSize;
        rhi::RHITexture *blurredTexture = m_gaussianBlurPass->execute(cmd, gaussianPc, brightTexture);

        CombineTexturePushConstant combineTexturePc;
        combineTexturePc.textureSize = brightnessPc.screenSize;
        combineTexturePc.intensity = settings.intensity;

        m_compositePass->execute(cmd, combineTexturePc, {textures.hdrTexture, blurredTexture, textures.bloomTexture});
    };

} // namespace nitro::renderer
