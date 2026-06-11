#include <nitro-rhi-backends/vulkan/vulkan-render-pass.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-texture.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>
#include <vector>
namespace nitro::rhi::vulkan
{
    VkAttachmentLoadOp convertToLoadOp(RenderPassDesc::LoadOp loadOp)
    {
        switch (loadOp)
        {
        case RenderPassDesc::LoadOp::Clear:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case RenderPassDesc::LoadOp::DontCare:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        case RenderPassDesc::LoadOp::Load:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        default:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        }
    }
    VkAttachmentStoreOp convertToStoreOp(RenderPassDesc::StoreOp storeOp)
    {
        switch (storeOp)
        {
        case RenderPassDesc::StoreOp::Store:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case RenderPassDesc::StoreOp::DontCare:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        default:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
    }
    VulkanRenderPass::VulkanRenderPass(VulkanDevice *device, const RenderPassDesc &desc) : m_device(device), width(desc.width), height(desc.height)
    {
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.colorAttachmentCount = 0;
        if (!desc.colorAttachments.empty())
        {

            for (auto &colorAttachment : desc.colorAttachments)
            {

                VulkanTexture *colorTexture = reinterpret_cast<VulkanTexture *>(colorAttachment.texture);
                VkRenderingAttachmentInfo colorAttachmentInfo{};
                colorAttachmentInfo.sType =
                    VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

                colorAttachmentInfo.imageView =
                    colorTexture->imageView;

                colorAttachmentInfo.imageLayout =
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                colorAttachmentInfo.loadOp =
                    convertToLoadOp(colorAttachment.load);

                colorAttachmentInfo.storeOp =
                    convertToStoreOp(colorAttachment.store);

                colorAttachmentInfo.clearValue.color =
                    {{colorAttachment.clearColor[0], colorAttachment.clearColor[1], colorAttachment.clearColor[2], colorAttachment.clearColor[3]}};

                colorTextures.push_back(colorTexture);
                colorAttachments.push_back(colorAttachmentInfo);
            }
            renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
            renderingInfo.pColorAttachments = colorAttachments.data();
        }

        if (desc.depthAttachment != nullptr)
        {
            depthTexture = reinterpret_cast<VulkanTexture *>(desc.depthAttachment->texture);

            depthAttachment.sType =
                VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

            depthAttachment.imageView =
                depthTexture->imageView;

            depthAttachment.imageLayout =
                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

            depthAttachment.loadOp =
                convertToLoadOp(desc.depthAttachment->load);

            depthAttachment.storeOp =
                convertToStoreOp(desc.depthAttachment->store);

            depthAttachment.clearValue.depthStencil =
                {desc.depthAttachment->clearDepth, 0};
            renderingInfo.pDepthAttachment = &depthAttachment;
        }
        renderingInfo.layerCount = 1;
        renderingInfo.renderArea.offset = {0, 0};
    }

    VulkanRenderPass::~VulkanRenderPass()
    {
    }

    void VulkanRenderPass::startTransition(VkCommandBuffer cmd)
    {
        if (depthTexture != nullptr)
        {
            m_device->transitionImageLayout(
                cmd,
                depthTexture->image,
                0,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                depthTexture->currentLayout,
                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT);
            depthTexture->currentLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        }
        for (auto &colorTexture : colorTextures)
        {
            m_device->transitionImageLayout(
                cmd,
                colorTexture->image,
                0,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                colorTexture->currentLayout,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT);
            colorTexture->currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
    }
    void VulkanRenderPass::endTransition(VkCommandBuffer cmd)
    {

        if (depthTexture != nullptr)
        {
            m_device->transitionImageLayout(
                cmd,
                depthTexture->image,
                0,
                VK_ACCESS_SHADER_READ_BIT,
                depthTexture->currentLayout,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT);
            depthTexture->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        for (auto &colorTexture : colorTextures)
        {
            m_device->transitionImageLayout(
                cmd,
                colorTexture->image,
                0,
                VK_ACCESS_SHADER_READ_BIT,
                colorTexture->currentLayout,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT);
            colorTexture->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }
} // namespace nitro::rhi::vulkan
