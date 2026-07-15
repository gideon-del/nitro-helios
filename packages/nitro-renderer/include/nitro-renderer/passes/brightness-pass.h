#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>
namespace nitro::renderer
{
    struct BrightnessPassPushConstant
    {
        glm::vec2 screenSize;
        float threshold;
        float pad;
    };
    struct BrightnessPassResource
    {
        rhi::RHIDescriptorSet *descriptorSet;
    };
    class BrightnessPass
    {
    public:
        BrightnessPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal);
        ~BrightnessPass();

        void resize(uint32_t width, uint32_t height);
        rhi::RHITexture *execute(rhi::RHICommandBuffer *cmd, BrightnessPassPushConstant pc, rhi::RHITexture *hdrScene);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        uint32_t m_width, m_height;
        rhi::RHIComputePipeline *m_computePipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIDescriptorSet *m_descriptorSet;
        rhi::RHITexture *m_brightnessTexture;
    };
} // namespace nitro::renderer
