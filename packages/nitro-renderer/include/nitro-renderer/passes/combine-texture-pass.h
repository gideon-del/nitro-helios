#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>

namespace nitro::renderer
{
    struct CombineTexturePushConstant
    {
        glm::vec2 textureSize;
        float intensity = 0.3;
        float _pad;
    };

    struct CombineTexturePassTextures
    {
        rhi::RHITexture *hdrTexture;
        rhi::RHITexture *blurredTexture;
        rhi::RHITexture *output;
    };
    class CombineTexturePass
    {
    public:
        CombineTexturePass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal);
        ~CombineTexturePass();
        void resize(uint32_t width, uint32_t height);
        void execute(rhi::RHICommandBuffer *cmd, CombineTexturePushConstant pc, CombineTexturePassTextures textures);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        uint32_t m_width, m_height;
        rhi::RHIComputePipeline *m_computePipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIDescriptorSet *m_descriptorSet;
    };
} // namespace nitro::renderer
