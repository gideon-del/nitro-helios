#pragma once
#include "nitro-renderer/per-frame.h"
#include "nitro-rhi/rhi.h"
#include "glm/glm.hpp"

namespace nitro::renderer
{
    struct CopyHizDepthPushConstant
    {
        glm::vec2 textureSize;
        float _pads[2];
    };

    struct CopyHizDepthResource
    {
        rhi::RHIDescriptorSet *descriptorSet;
        rhi::RHITexture *lastDepthTexture = nullptr;
        rhi::RHITexture *lastHizTexture = nullptr;
    };

    struct CopyHizDepthRGResource
    {
        rhi::RHITexture *depthTexture;
        rhi::RHITexture *hizTexture;
    };

    class CopyHizDepthPass
    {
    public:
        CopyHizDepthPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal);
        ~CopyHizDepthPass();
        void execute(rhi::RHICommandBuffer *cmd, CopyHizDepthPushConstant &pc, const CopyHizDepthRGResource &rgResources);

    private:
        bool isStaleDescriptorSet(CopyHizDepthResource &resource, const CopyHizDepthRGResource &rgResources);
        void bindDescriptorSet(CopyHizDepthResource &resource, const CopyHizDepthRGResource &rgResources);
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIComputePipeline *m_computePipeline;
        PerFrame<CopyHizDepthResource> m_resources;
    };
} // namespace nitro::renderer
