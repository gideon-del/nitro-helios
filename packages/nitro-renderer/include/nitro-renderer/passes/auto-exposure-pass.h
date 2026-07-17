#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>
#include <nitro-renderer/per-frame.h>
#include <nitro-renderer/settings.h>

namespace nitro::renderer
{
    struct AutoExposureMips
    {
        rhi::RHITexture *mip;
        rhi::RHIDescriptorSet *descriptorSet;
        uint32_t width, height;
    };

    struct ReadbackBuffer
    {
        rhi::RHIBuffer *buffer;
    };

    struct AutoExposurePushConstant
    {
        glm::vec2 inputTextureSize;
        glm::vec2 outputTextureSize;
    };
    class AutoExposurePass
    {
    public:
        AutoExposurePass(std::shared_ptr<rhi::RHIDevice> device,
                         uint32_t width,
                         uint32_t height,
                         std::string shaderDir,
                         bool isMetal);
        ~AutoExposurePass();
        void resize(uint32_t width, uint32_t height);
        float execute(rhi::RHICommandBuffer *cmd, AutoExposurePushConstant pc, rhi::RHITexture *inputTexture, ToneMapSettings &toneMapSettings,
                      float deltaTime);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        uint32_t m_width, m_height;
        rhi::RHIComputePipeline *m_luminanceComputePipeline;
        rhi::RHIComputePipeline *m_downSampleComputePipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;

        std::vector<AutoExposureMips> m_autoExposureMips;
        rhi::RHITexture *m_lastInputTexture = nullptr;
        void m_allocateMips();
        void m_destroyMips();
        PerFrame<ReadbackBuffer> m_readbackBuffers;
    };
} // namespace nitro::renderer
