#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>
#include <nitro-renderer/per-frame.h>
#include <nitro-renderer/render-graph.h>

namespace nitro::renderer
{

    struct SSAOResources
    {
        rhi::RHIDescriptorSet *descriptorSet;
        rhi::RHIBuffer *randomSampleBuffer;
    };

    struct SSAOPushConstant
    {
        glm::mat4 view;
        glm::mat4 invProj;
        glm::mat4 proj;
        glm::vec2 textureSize;
        uint totalSamples;
        float radius = 0.2f;
    };
    struct SSAOBlurPushConstant
    {
        glm::vec2 textureSize;
        float depthSigma = 1.0f;
        float _pads;
    };
    struct SSAOPassTextureIDs
    {
        RGTextureID gDepth;
        RGTextureID gNormal;
        RGTextureID ssaoTex;
    };
    class SSAOPass
    {
    public:
        SSAOPass(std::shared_ptr<rhi::RHIDevice> device,
                 uint32_t width,
                 uint32_t height,
                 std::string shaderDir,
                 bool isMetal);
        ~SSAOPass();
        void resize(uint32_t width,
                    uint32_t height);
        void execute(rhi::RHICommandBuffer *cmd, SSAOPushConstant pc, const RGResources &resources, const RGResourceID blurredSSAO, const std::vector<glm::vec4> &samples, float depthSigma = 0.3f);
        void bindResources(const RGResources &resources, const SSAOPassTextureIDs &textures);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        uint32_t m_width, m_height;
        rhi::RHIDescriptorLayout *m_ssaoDescriptorLayout;
        rhi::RHIComputePipeline *m_ssaoComputePipeline;

        rhi::RHIDescriptorLayout *m_blurDescriptorLayout;
        rhi::RHIComputePipeline *m_blurComputePipeline;
        rhi::RHIDescriptorSet *m_blurDescriptorSet;

        rhi::RHITexture *m_ssaoTexture;
        rhi::RHITexture *m_noiseTexture;

        PerFrame<SSAOResources> m_resources;
    };
} // namespace nitro::renderer
