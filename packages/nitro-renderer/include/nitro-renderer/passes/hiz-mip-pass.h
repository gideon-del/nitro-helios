#pragma once
#include <nitro-rhi/rhi.h>
#include "nitro-renderer/per-frame.h"
#include "nitro-renderer/render-graph.h"

namespace nitro::renderer
{

    static constexpr uint32_t HIZ_MIP_COUNT = 8;
    struct HizMipResource
    {
        std::array<rhi::RHIDescriptorSet *, HIZ_MIP_COUNT> descriptorSets{};
    };
    struct HizMipPushConstant
    {
        glm::vec2 textureSize;
        glm::vec2 _pad;
    };
    class HiZMipPass
    {
    public:
        HiZMipPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal);
        ~HiZMipPass();
        void bindResources(const RGResources &resources, const RGTextureID &hizTexID);
        void execute(rhi::RHICommandBuffer *cmd, const uint32_t width, const uint32_t height, rhi::RHITexture *hizTexture);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIComputePipeline *m_computePipeline;
        PerFrame<HizMipResource> m_resources;
    };
} // namespace nitro::renderer
