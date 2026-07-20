#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>

namespace nitro::renderer
{
    struct FXAAPushConstant
    {
        glm::vec2 textureSize;
        float contrastThreshold = 0.0312f;
    };
    class FXAAPass
    {
    public:
        FXAAPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, rhi::RHITexture *toneMapTexture, std::string shaderDir, bool isMetal);
        ~FXAAPass();
        void resize(uint32_t width, uint32_t height, rhi::RHITexture *toneMapTexture);
        void execute(rhi::RHICommandBuffer *cmd, FXAAPushConstant pc);
        rhi::RHITexture *getFXAATexture() { return m_fxaaTexture; }

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        uint32_t m_width, m_height;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIComputePipeline *m_computePipeline;
        rhi::RHIDescriptorSet *m_descriptorSet;
        rhi::RHITexture *m_fxaaTexture;
    };
} // namespace nitro::renderer
