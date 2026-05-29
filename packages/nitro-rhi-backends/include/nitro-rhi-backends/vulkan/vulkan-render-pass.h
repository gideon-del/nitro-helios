#pragma once
#include <nitro-rhi/rhi-render-pass.h>
#include <vulkan/vulkan.h>
#include <vector>
namespace nitro::rhi::vulkan
{

    class VulkanDevice;
    class VulkanTexture;
    class VulkanRenderPass : public RHIRenderPass
    {
    public:
        VulkanRenderPass(VulkanDevice *device, const RenderPassDesc &desc);

        ~VulkanRenderPass() override;

        VkRenderingAttachmentInfo colorAttachment;
        VkRenderingAttachmentInfo depthAttachment;

        VkRenderingInfo renderingInfo;
        VulkanTexture *depthTexture = nullptr;
        VulkanTexture *colorTexture = nullptr;

        void startTransition(VkCommandBuffer cmd);
        void endTransition(VkCommandBuffer cmd);

    private:
        VulkanDevice *m_device;
    };
} // namespace nitro::rhi::vulkan
