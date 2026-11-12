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
                    convertResourceStateToImageLayout(ResourceState::RenderTarget);

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

            if (desc.depthAttachment->depthWrite)
                depthState = ResourceState::DepthWrite;
            else if (desc.depthAttachment->stencilWrite)
                depthState = ResourceState::DepthReadStencilWrite;
            else
                depthState = ResourceState::DepthRead;
            m_dsLayout = desc.depthAttachment->depthWrite
                             ? convertResourceStateToImageLayout(ResourceState::DepthWrite)
                             : convertResourceStateToImageLayout(ResourceState::DepthRead);
            depthAttachment.sType =
                VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

            depthAttachment.imageView =
                depthTexture->imageView;

            depthAttachment.imageLayout = convertResourceStateToImageLayout(depthState);
            ;

            depthAttachment.loadOp =
                convertToLoadOp(desc.depthAttachment->load);

            depthAttachment.storeOp =
                convertToStoreOp(desc.depthAttachment->store);

            depthAttachment.clearValue.depthStencil =
                {desc.depthAttachment->clearDepth, desc.depthAttachment->clearStencil};
            renderingInfo.pDepthAttachment = &depthAttachment;

            if (desc.depthAttachment->hasStencil)
            {
                stencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                stencilAttachment.imageView =
                    depthTexture->imageView;

                stencilAttachment.imageLayout = convertResourceStateToImageLayout(depthState);

                stencilAttachment.loadOp =
                    convertToLoadOp(desc.depthAttachment->stencilLoad);

                stencilAttachment.storeOp =
                    convertToStoreOp(desc.depthAttachment->stencilStore);

                stencilAttachment.clearValue.depthStencil =
                    {desc.depthAttachment->clearDepth, desc.depthAttachment->clearStencil};

                renderingInfo.pStencilAttachment = &stencilAttachment;
            }
            m_dstAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            if (desc.depthAttachment->depthWrite || desc.depthAttachment->stencilWrite)
                m_dstAccess |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }
        renderingInfo.layerCount = 1;
        renderingInfo.renderArea.offset = {0, 0};
        renderingInfo.renderArea.extent = {width, height};
    }

    VulkanRenderPass::~VulkanRenderPass()
    {
    }

} // namespace nitro::rhi::vulkan
