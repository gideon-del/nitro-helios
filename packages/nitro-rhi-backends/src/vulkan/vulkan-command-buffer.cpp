#include <nitro-rhi-backends/vulkan/vulkan-command-buffer.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-swapchain.h>
#include <nitro-rhi-backends/vulkan/vulkan-pipeline.h>
#include <nitro-rhi-backends/vulkan/vulkan-buffer.h>
#include <nitro-rhi-backends/vulkan/vulkan-texture.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>

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

        VkRenderPassBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.framebuffer = swapchain->framebuffers[m_imageIdx];
        beginInfo.renderPass = m_device->defaultRenderPass;
        beginInfo.clearValueCount = 1;
        beginInfo.renderArea = {
            .offset = {
                .x = 0,
                .y = 0},
            .extent = {.width = m_device->surface->getWidth(), .height = m_device->surface->getHeight()}

        };

        std::array<VkClearValue, 2> clearValues;
        clearValues[0].color = {{desc.clearColor[0], desc.clearColor[1], desc.clearColor[2], desc.clearColor[3]}};
        clearValues[1].depthStencil = {desc.clearDepth, 0};
        beginInfo.pClearValues = clearValues.data();
        beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

        vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    };

    void VulkanCommandBuffer::endRenderPass()
    {
        vkCmdEndRenderPass(cmd);
    }
    void VulkanCommandBuffer::bindPipeline(RHIPipeline *pipeline)
    {
        VulkanPipeline *vulkanPipeline = reinterpret_cast<VulkanPipeline *>(pipeline);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->pipeline);
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
        vkEndCommandBuffer(cmd);

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
}