#pragma once
#include <nitro-rhi/rhi-render-pass.h>
#include <vulkan/vulkan.h>
#include <vector>
namespace nitro::rhi::vulkan
{

    inline VkImageLayout depthStencilLayout(bool depthWrite, bool stencilWrite)
    {
        if (depthWrite && stencilWrite)
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        if (depthWrite && !stencilWrite)
            return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
        if (!depthWrite && stencilWrite)
            return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }
    class VulkanDevice;
    class VulkanTexture;
    class VulkanRenderPass : public RHIRenderPass
    {
    public:
        VulkanRenderPass(VulkanDevice *device, const RenderPassDesc &desc);

        ~VulkanRenderPass() override;

        std::vector<VkRenderingAttachmentInfo> colorAttachments;
        VkRenderingAttachmentInfo depthAttachment;
        VkRenderingAttachmentInfo stencilAttachment;

        VkRenderingInfo renderingInfo;
        VulkanTexture *depthTexture = nullptr;
        std::vector<VulkanTexture *> colorTextures;
        uint32_t width;
        uint32_t height;
        void startTransition(VkCommandBuffer cmd);
        void endTransition(VkCommandBuffer cmd);

    private:
        VulkanDevice *m_device;
        VkImageLayout m_dsLayout;
        VkAccessFlags m_dstAccess;
    };
} // namespace nitro::rhi::vulkan
