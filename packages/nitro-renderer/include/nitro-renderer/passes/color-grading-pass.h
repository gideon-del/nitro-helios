#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>
#include <nitro-renderer/per-frame.h>
#include <nitro-renderer/single-texture-pass-resource.h>

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

    struct ColorGradingTextures
    {
        rhi::RHITexture *sceneTexture;
        rhi::RHITexture *output;
    };

    class ColorGradingPass
    {
    public:
        ColorGradingPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal);
        ~ColorGradingPass();
        void resize(uint32_t width, uint32_t height);
        void execute(rhi::RHICommandBuffer *cmd, ColorGradingPushConstant pc, ColorGradingTextures textures);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        uint32_t m_width, m_height;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIComputePipeline *m_computePipeline;
        PerFrame<SingleInputPassResource> m_resources;
        };
} // namespace nitro::renderer
