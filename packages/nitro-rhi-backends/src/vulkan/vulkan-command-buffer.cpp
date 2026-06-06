#include <nitro-rhi-backends/vulkan/vulkan-command-buffer.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-swapchain.h>
#include <nitro-rhi-backends/vulkan/vulkan-pipeline.h>
#include <nitro-rhi-backends/vulkan/vulkan-buffer.h>
#include <nitro-rhi-backends/vulkan/vulkan-texture.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>
#include <nitro-rhi-backends/vulkan/vulkan-render-pass.h>
#include <nitro-rhi-backends/vulkan/vulkan-descriptor-set.h>

namespace nitro::rhi::vulkan
{
    VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice *device, VulkanSwapchain *swapchain,
                                             VkCommandBuffer cmd, uint32_t frameIdx) : m_device(device), swapchain(swapchain), cmd(cmd), m_frameIdx(frameIdx)
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        checkVkResult(vkCreateSemaphore(m_device->device, &semaphoreInfo, nullptr, &imageAvailable), "Semaphore not created");

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        checkVkResult(vkCreateFence(m_device->device, &fenceInfo, nullptr, &inFlight), "Fence not created");
    }
    VulkanCommandBuffer::~VulkanCommandBuffer()
    {

        if (cmd != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(m_device->device, m_device->commandPool, 1, &cmd);
        }

        if (inFlight != VK_NULL_HANDLE)
        {
            vkDestroyFence(m_device->device, inFlight, nullptr);
        }

        if (imageAvailable != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(m_device->device, imageAvailable, nullptr);
        }
    };

    void VulkanCommandBuffer::beginRenderPass(const RHIRenderPassDesc &desc)
    {

        m_device->transitionImageLayout(
            cmd,
            swapchain->backBuffers[m_imageIdx]->image,
            0,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            swapchain->backBuffers[m_imageIdx]->currentLayout,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
        swapchain->backBuffers[m_imageIdx]->currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType =
            VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

        colorAttachment.imageView = swapchain->backBuffers[m_imageIdx]->imageView;

        colorAttachment.imageLayout =
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        colorAttachment.loadOp =
            VK_ATTACHMENT_LOAD_OP_CLEAR;

        colorAttachment.storeOp =
            VK_ATTACHMENT_STORE_OP_STORE;

        colorAttachment.clearValue.color =
            {{desc.clearColor[0], desc.clearColor[1], desc.clearColor[2], desc.clearColor[3]}};

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType =
            VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

        depthAttachment.imageView =
            swapchain->depthTexture->imageView;

        depthAttachment.imageLayout =
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

        depthAttachment.loadOp =
            VK_ATTACHMENT_LOAD_OP_CLEAR;

        depthAttachment.storeOp =
            VK_ATTACHMENT_STORE_OP_STORE;

        depthAttachment.clearValue.depthStencil =
            {desc.clearDepth, 0};
        VkRenderingInfo renderingInfo{};
        renderingInfo.sType =
            VK_STRUCTURE_TYPE_RENDERING_INFO;

        renderingInfo.renderArea.extent =
            swapchain->extent;

        renderingInfo.layerCount = 1;

        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments =
            &colorAttachment;

        renderingInfo.pDepthAttachment =
            &depthAttachment;
        vkCmdResetQueryPool(cmd, m_device->queryPool, 0, 2);
        vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_device->queryPool, 0);
        vkCmdBeginRendering(cmd, &renderingInfo);
    };
    void VulkanCommandBuffer::beginRenderPass(RHIRenderPass *renderPass)
    {
        VulkanRenderPass *vulkanRenderPass = reinterpret_cast<VulkanRenderPass *>(renderPass);

        vulkanRenderPass->startTransition(cmd);
        vulkanRenderPass->renderingInfo.renderArea.extent = swapchain->extent;
        if (vulkanRenderPass->depthTexture)
        {
            vulkanRenderPass->renderingInfo.renderArea.extent = {vulkanRenderPass->depthTexture->width, vulkanRenderPass->depthTexture->height};
        }

        vkCmdBeginRendering(cmd, &vulkanRenderPass->renderingInfo);

        m_activeRenderPass = vulkanRenderPass;
    }
    void VulkanCommandBuffer::endRenderPass()
    {
        vkCmdEndRendering(cmd);

        if (m_activeRenderPass == nullptr)
        {
            vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_device->queryPool, 1);
            return;
        }

        m_activeRenderPass->endTransition(cmd);

        m_activeRenderPass = nullptr;
    }
    void VulkanCommandBuffer::bindPipeline(RHIPipeline *pipeline)
    {
        VulkanPipeline *vulkanPipeline = reinterpret_cast<VulkanPipeline *>(pipeline);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->pipeline);

        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        if (m_activeRenderPass != nullptr)
        {
            viewport.width = (float)m_activeRenderPass->width;
            viewport.height = (float)m_activeRenderPass->height;
        }
        else
        {
            viewport.width = (float)swapchain->extent.width;
            viewport.height = (float)swapchain->extent.height;
        }

        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        m_pipeline = vulkanPipeline;
        vkCmdSetViewport(
            cmd,
            0,
            1,
            &viewport);

        VkRect2D scissors{};

        if (m_activeRenderPass != nullptr)
        {
            scissors.extent = {
                m_activeRenderPass->width,
                m_activeRenderPass->height};
        }
        else
        {
            scissors.extent = swapchain->extent;
        }

        scissors.offset = {0, 0};

        vkCmdSetScissor(
            cmd,
            0,
            1,
            &scissors);
    }

    void VulkanCommandBuffer::bindVertexBuffer(RHIBuffer *buffer)
    {
        VulkanBuffer *vertexBuffer = reinterpret_cast<VulkanBuffer *>(buffer);

        VkDeviceSize offset[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer->buffer, offset);
    }
    void VulkanCommandBuffer::bindIndexBuffer(RHIBuffer *buffer)
    {
        VulkanBuffer *indexBuffer = reinterpret_cast<VulkanBuffer *>(buffer);

        VkDeviceSize offset[] = {0};
        vkCmdBindIndexBuffer(cmd, indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanCommandBuffer::bindDescriptorSet(RHIDescriptorSet *set, uint32_t binding)
    {

        if (!m_pipeline)
        {
            throw std::runtime_error("Must bind pipeline before descriptor set");
        }
        VulkanDescriptorSet *vkSet = reinterpret_cast<VulkanDescriptorSet *>(set);
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipeline->layout,
            binding,
            1,
            &vkSet->descriptorSet,
            0,
            nullptr);
    }

    void VulkanCommandBuffer::draw(uint32_t vertexCount)
    {
        vkCmdDraw(
            cmd,
            vertexCount,
            1,
            0,
            0);
    }
    void VulkanCommandBuffer::drawIndexed(uint32_t indexCount)
    {
        vkCmdDrawIndexed(
            cmd,
            indexCount,
            1,
            0,
            0,
            0);
    }
    void VulkanCommandBuffer::present()
    {

        m_device->transitionImageLayout(
            cmd,
            swapchain->backBuffers[m_imageIdx]->image,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            0,
            swapchain->backBuffers[m_imageIdx]->currentLayout,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
        swapchain->backBuffers[m_imageIdx]->currentLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        checkVkResult(vkEndCommandBuffer(cmd), "Command Buffer not ended");
        ;

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &imageAvailable;
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &swapchain->renderFinished[m_imageIdx];
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        checkVkResult(vkQueueSubmit(m_device->graphicsQueue, 1, &submitInfo,
                                    inFlight),
                      "Queue submit failed");
        VkPresentInfoKHR presentInfo{};

        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain->swapchain;
        presentInfo.pWaitSemaphores = &swapchain->renderFinished[m_imageIdx];
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pImageIndices = &m_imageIdx;
        VkResult presentResult = vkQueuePresentKHR(m_device->presentQueue, &presentInfo);

        uint64_t timestamps[2];

        vkGetQueryPoolResults(
            m_device->device,
            m_device->queryPool,
            0,
            2,
            sizeof(timestamps),
            timestamps,
            sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT

        );

        // float gpuMs = (timestamps[1] - timestamps[0]) * m_device->timestampPeriod / 1e6f;
        // std::cout << "Main Pass: " << gpuMs << " ms" << std::endl;
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
            presentResult == VK_SUBOPTIMAL_KHR)
        {
            swapchain->resize(
                m_device->surface->getWidth(),
                m_device->surface->getHeight());
        }
    }
    void VulkanCommandBuffer::bindUniformBuffer(RHIBuffer *buffer, uint32_t binding)
    {
    }

    void VulkanCommandBuffer::setPushConstant(void *data, size_t size, uint32_t binding)
    {
        if (!m_pipeline)
        {
            throw std::runtime_error("Vulkan pipeline not found");
        }

        vkCmdPushConstants(
            cmd,
            m_pipeline->layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            size,
            data);
    }
}