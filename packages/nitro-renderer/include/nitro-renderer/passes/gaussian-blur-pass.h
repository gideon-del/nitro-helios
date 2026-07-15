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
    class GaussianBlurPass
    {
    public:
        GaussianBlurPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal);
        ~GaussianBlurPass();
        void resize(uint32_t width, uint32_t height);
        rhi::RHITexture *execute(rhi::RHICommandBuffer *cmd, GaussianBlurPushConstant pc, rhi::RHITexture *inputTexture);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        uint32_t m_width, m_height;
        rhi::RHIComputePipeline *m_computePipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIDescriptorSet *m_horizontalDescriptorSet;
        rhi::RHIDescriptorSet *m_verticalDescriptorSet;
        rhi::RHITexture *m_horizontalTexture;
        rhi::RHITexture *m_verticalTexture;
    };
} // namespace nitro::renderer
