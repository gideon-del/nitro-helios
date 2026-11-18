#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>

namespace nitro::renderer
{
    struct GaussianBlurPushConstant
    {
        glm::vec2 inputTextureSize;
        glm::vec2 outputTextureSize;
        uint horizontal = 1;
        float _pad[3];
    };

    struct UpSamplePushConstant
    {
        glm::vec2 textureSize;
        float _pad[2];
    };

    struct GaussianBlurMip
    {
        rhi::RHITexture *texture;
        rhi::RHITexture *horizontalScratch;
        rhi::RHIDescriptorSet *horizontalDescriptorSet;
        rhi::RHIDescriptorSet *verticalDescriptorSet;
        uint32_t width;
        uint32_t height;
    };

    class GaussianBlurPass
    {
    public:
        GaussianBlurPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal);
        ~GaussianBlurPass();
        void resize(uint32_t width, uint32_t height);
        rhi::RHITexture *execute(rhi::RHICommandBuffer *cmd, GaussianBlurPushConstant pc, rhi::RHITexture *inputTexture);
        static constexpr uint32_t GAUSSIAN_MIP_COUNT = 4;

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        uint32_t m_width, m_height;
        rhi::RHIComputePipeline *m_downSampleComputePipeline;
        rhi::RHIDescriptorLayout *m_downSampleDescriptorLayout;

        rhi::RHIComputePipeline *m_upSampleComputePipeline;
        rhi::RHIDescriptorLayout *m_upSampleDescriptorLayout;
        rhi::RHITexture *m_lastInputTexture = nullptr;
        std::array<GaussianBlurMip, GAUSSIAN_MIP_COUNT> m_downSampleBlurMips;
        std::array<GaussianBlurMip, GAUSSIAN_MIP_COUNT> m_upSampleBlurMips;
    };
} // namespace nitro::renderer
