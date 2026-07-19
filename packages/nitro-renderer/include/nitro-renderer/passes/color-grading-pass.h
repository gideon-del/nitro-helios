#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>

namespace nitro::renderer
{

    struct ColorGradingPushConstant
    {
        glm::vec4 lift;
        glm::vec4 gamma;
        glm::vec4 gain;
        glm::vec2 textureSize;
        float _pads[2];
    };

    class ColorGradingPass
    {
    public:
        ColorGradingPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal);
        ~ColorGradingPass();
        void resize(uint32_t width, uint32_t height);
        rhi::RHITexture *execute(rhi::RHICommandBuffer *cmd, ColorGradingPushConstant pc, rhi::RHITexture *hdrTexture);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        uint32_t m_width, m_height;
        rhi::RHITexture *m_lastHdrTexture = nullptr;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIComputePipeline *m_computePipeline;
        rhi::RHIDescriptorSet *m_descriptorSet;
        rhi::RHITexture *m_colorGradedTexture;
    };
} // namespace nitro::renderer
