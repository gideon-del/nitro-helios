#include <nitro-rhi-backends/vulkan/vulkan-command-buffer.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-swapchain.h>
#include <nitro-rhi-backends/vulkan/vulkan-pipeline.h>
#include <nitro-rhi-backends/vulkan/vulkan-buffer.h>
#include <nitro-rhi-backends/vulkan/vulkan-texture.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>
#include <nitro-rhi-backends/vulkan/vulkan-render-pass.h>
#include <nitro-rhi-backends/vulkan/vulkan-descriptor-set.h>
#include <nitro-rhi-backends/vulkan/vulkan-compute-pipeline.h>

namespace nitro::rhi::vulkan
{

    VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice *device, VulkanSwapchain *swapchain,
                                             VkCommandBuffer cmd, uint32_t frameIdx, bool isOneTime) : m_device(device), swapchain(swapchain), cmd(cmd), m_frameIdx(frameIdx)
    {
        if (isOneTime)
            return;
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

        discard(*swapchain->backBuffers[m_imageIdx]);
        TextureBarrier textureBarrier;
        textureBarrier.texture = swapchain->backBuffers[m_imageIdx];
        textureBarrier.to = ResourceState::RenderTarget;

        TextureBarrier depthTextureBarrier;
        depthTextureBarrier.texture = swapchain->depthTexture;
        depthTextureBarrier.to = ResourceState::DepthWrite;
        barrier({{textureBarrier, depthTextureBarrier}});

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
            convertResourceStateToImageLayout(ResourceState::DepthWrite);

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

        vkCmdBeginRendering(cmd, &renderingInfo);
    };
    void VulkanCommandBuffer::beginRenderPass(RHIRenderPass *renderPass)
    {
        VulkanRenderPass *vulkanRenderPass = reinterpret_cast<VulkanRenderPass *>(renderPass);

        for (auto texture : vulkanRenderPass->colorTextures)
        {
            const auto &state = texture->state();

            auto resolvedRange = TextureSubresource::all().resolve(state.mipCount(), state.layerCount());

            assert(state.isRangeIn(ResourceState::RenderTarget,
                                   resolvedRange.baseMip,
                                   resolvedRange.mipCount,
                                   resolvedRange.baseLayer,
                                   resolvedRange.layerCount) &&
                   "barrier to RenderTarget before begining a render pass");
        }

        if (vulkanRenderPass->depthTexture)
        {

            const auto &state = vulkanRenderPass->depthTexture->state();

            auto resolvedRange = TextureSubresource::all().resolve(state.mipCount(), state.layerCount());

            assert(state.isRangeIn(vulkanRenderPass->depthState,
                                   resolvedRange.baseMip,
                                   resolvedRange.mipCount,
                                   resolvedRange.baseLayer,
                                   resolvedRange.layerCount) &&
                   "barrier to DepthState before begining a render pass");
        }

        vkCmdBeginRendering(cmd, &vulkanRenderPass->renderingInfo);

        m_activeRenderPass = vulkanRenderPass;
    }
    void VulkanCommandBuffer::endRenderPass()
    {
        vkCmdEndRendering(cmd);

        m_activeRenderPass = nullptr;
    }
    void VulkanCommandBuffer::bindPipeline(RHIPipeline *pipeline)
    {
        VulkanPipeline *vulkanPipeline = reinterpret_cast<VulkanPipeline *>(pipeline);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->pipeline);

        m_pipeline = vulkanPipeline;
    }

    void VulkanCommandBuffer::setViewPort(const RHIViewport &viewport)
    {
        VkViewport vulkanViewport{};
        vulkanViewport.x = viewport.x;
        vulkanViewport.y = viewport.y;
        vulkanViewport.width = viewport.width;
        vulkanViewport.height = viewport.height;
        vulkanViewport.minDepth = viewport.minDepth;
        vulkanViewport.maxDepth = viewport.maxDepth;

        vkCmdSetViewport(
            cmd,
            0,
            1,
            &vulkanViewport);
    }

    void VulkanCommandBuffer::setScissor(const RHIScissor &scissor)
    {

        VkRect2D vulkanScissor;

        vulkanScissor.offset.x = scissor.x;
        vulkanScissor.offset.y = scissor.y;
        vulkanScissor.extent.width = scissor.width;
        vulkanScissor.extent.height = scissor.height;
        vkCmdSetScissor(
            cmd,
            0,
            1,
            &vulkanScissor);
    };

    void VulkanCommandBuffer::setStencilReference(uint32_t reference)
    {
        vkCmdSetStencilReference(
            cmd,
            VK_STENCIL_FRONT_AND_BACK,
            reference);
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
    void VulkanCommandBuffer::bindComputeDescriptorSet(RHIDescriptorSet *set, uint32_t binding)
    {

        if (!m_computePipeline)
        {
            throw std::runtime_error("Must bind compute pipeline before descriptor set");
        }

        VulkanDescriptorSet *vkSet = reinterpret_cast<VulkanDescriptorSet *>(set);
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            m_computePipeline->layout,
            binding,
            1,
            &vkSet->descriptorSet,
            0,
            nullptr);
    }

    void VulkanCommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount)
    {
        m_FrameStats.triangles += vertexCount / 3;
        m_FrameStats.drawCalls += 1;
        vkCmdDraw(
            cmd,
            vertexCount,
            instanceCount,
            0,
            0);
    }
    void VulkanCommandBuffer::drawIndirect(RHIBuffer *indirectBuffer, size_t offset)
    {

        VulkanBuffer *vulkanIndirectBuffer = reinterpret_cast<VulkanBuffer *>(indirectBuffer);

        vkCmdDrawIndirect(
            cmd,
            vulkanIndirectBuffer->buffer,
            offset,
            1,
            sizeof(VkDrawIndirectCommand));
    }
    void VulkanCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount)
    {
        m_FrameStats.triangles += indexCount / 3;
        m_FrameStats.drawCalls += 1;
        vkCmdDrawIndexed(
            cmd,
            indexCount,
            instanceCount,
            0,
            0,
            0);
    }
    void VulkanCommandBuffer::dispatchIndirect(RHIBuffer *indirectBuffer, size_t offset)
    {

        VulkanBuffer *vulkanIndirectBuffer = reinterpret_cast<VulkanBuffer *>(indirectBuffer);

        vkCmdDispatchIndirect(
            cmd,
            vulkanIndirectBuffer->buffer,
            offset);
    }

    void VulkanCommandBuffer::submit()
    {
        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        checkVkResult(vkQueueSubmit(m_device->graphicsQueue, 1, &submitInfo,
                                    VK_NULL_HANDLE),
                      "Queue submit failed");
    };
    void VulkanCommandBuffer::present()
    {

        TextureBarrier textureBarrier;
        textureBarrier.texture = swapchain->backBuffers[m_imageIdx];
        textureBarrier.to = ResourceState::Present;

        barrier({{textureBarrier}});

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

        // uint64_t timestamps[2];

        // vkGetQueryPoolResults(
        //     m_device->device,
        //     m_device->queryPool,
        //     0,
        //     2,
        //     sizeof(timestamps),
        //     timestamps,
        //     sizeof(uint64_t),
        //     VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT

        // );

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

    FrameStats VulkanCommandBuffer::getFrameStats()
    {
        return m_FrameStats;
    }
    void VulkanCommandBuffer::resetFrameStats()
    {
        m_FrameStats.drawCalls = 0;
        m_FrameStats.triangles = 0;
        m_FrameStats.vertices = 0;

        m_FrameStats.imageBarriers = 0;
        m_FrameStats.bufferBarriers = 0;
        m_FrameStats.barrierCalls = 0;
    }
    void VulkanCommandBuffer::updateVertexCount(uint32_t count)
    {
        m_FrameStats.vertices += count;
    }
    void VulkanCommandBuffer::setPushConstant(void *data, size_t size, uint32_t binding, bool isCompute)
    {

        if (isCompute)
        {
            if (!m_computePipeline)
            {
                throw std::runtime_error("Vulkan compute pipeline not found");
            }

            vkCmdPushConstants(
                cmd,
                m_computePipeline->layout,
                VK_SHADER_STAGE_COMPUTE_BIT,
                0,
                size,
                data);
            return;
        }
        if (!m_pipeline)
        {
            throw std::runtime_error("Vulkan pipeline not found");
        }

        vkCmdPushConstants(
            cmd,
            m_pipeline->layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            size,
            data);
    }

    void VulkanCommandBuffer::bindComputePipeline(RHIComputePipeline *pipeline)
    {
        VulkanComputePipeline *computePipeline = reinterpret_cast<VulkanComputePipeline *>(pipeline);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline->pipeline);
        m_computePipeline = computePipeline;
    }

    void VulkanCommandBuffer::dispatch(uint32_t x, uint32_t y, uint32_t z)
    {
        vkCmdDispatch(
            cmd,
            x,
            y,
            z);
    }

    void VulkanCommandBuffer::generateMipmaps(RHITexture *texture)
    {
        VulkanTexture *vulkanTexture = reinterpret_cast<VulkanTexture *>(texture);
        int i = 1;
        int totalLayer = vulkanTexture->isCubeMap() ? 6 : 1;
        if (vulkanTexture->mipmapLevels <= 1)
            return;

        uint32_t texWidth = vulkanTexture->width;
        uint32_t texHeight = vulkanTexture->height;

        TextureBarrier textureBarrier;
        textureBarrier.texture = texture;
        textureBarrier.range.baseMip = 0;
        textureBarrier.range.mipCount = 1;
        textureBarrier.to = ResourceState::CopySrc;
        barrier({{textureBarrier}});

        while (i < vulkanTexture->mipmapLevels)
        {

            textureBarrier.range.baseMip = i;
            textureBarrier.range.mipCount = 1;
            textureBarrier.to = ResourceState::CopyDst;
            barrier({{textureBarrier}});

            VkImageBlit imageBlit{};
            imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlit.srcSubresource.baseArrayLayer = 0;
            imageBlit.srcSubresource.layerCount = totalLayer;
            imageBlit.srcSubresource.mipLevel = i - 1;

            imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlit.dstSubresource.baseArrayLayer = 0;
            imageBlit.dstSubresource.layerCount = totalLayer;
            imageBlit.dstSubresource.mipLevel = i;

            imageBlit.srcOffsets[1].x = std::max(int32_t(texWidth >> (i - 1)), 1);
            imageBlit.srcOffsets[1].y = std::max(int32_t(texHeight >> (i - 1)), 1);
            imageBlit.srcOffsets[1].z = 1;

            imageBlit.dstOffsets[1].x = std::max(int32_t(texWidth >> i), 1);
            imageBlit.dstOffsets[1].y = std::max(int32_t(texHeight >> i), 1);
            imageBlit.dstOffsets[1].z = 1;

            for (uint32_t face = 0; face < totalLayer; ++face)
            {
                imageBlit.srcSubresource.baseArrayLayer = face;
                imageBlit.srcSubresource.layerCount = 1;

                imageBlit.dstSubresource.baseArrayLayer = face;
                imageBlit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(
                    cmd,
                    vulkanTexture->image,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    vulkanTexture->image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &imageBlit,
                    VK_FILTER_LINEAR);
            }

            textureBarrier.to = ResourceState::CopySrc;
            barrier({{textureBarrier}});

            i++;
        }

        textureBarrier.to = ResourceState::ShaderRead;
        textureBarrier.range = TextureSubresource::all();
        barrier({{textureBarrier}});
    }

    void VulkanCommandBuffer::copyTextureToBuffer(RHITexture *texture, RHIBuffer *buffer)
    {
        VulkanBuffer *vulkanBuffer = reinterpret_cast<VulkanBuffer *>(buffer);
        VulkanTexture *vulkanTexture = reinterpret_cast<VulkanTexture *>(texture);

        VkBufferImageCopy2 region{};
        region.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
        region.bufferImageHeight = vulkanTexture->height;
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.imageExtent = {1, 1, 1};
        region.imageSubresource.aspectMask = vulkanTexture->imageAspect;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageSubresource.mipLevel = 0;

        VkCopyImageToBufferInfo2 bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2;
        bufferInfo.srcImage = vulkanTexture->image;
        bufferInfo.srcImageLayout = convertResourceStateToImageLayout(ResourceState::CopySrc);
        bufferInfo.regionCount = 1;
        bufferInfo.pRegions = &region;
        bufferInfo.dstBuffer = vulkanBuffer->buffer;

        vkCmdCopyImageToBuffer2(
            cmd,
            &bufferInfo);
    }

    void VulkanCommandBuffer::fillBuffer(RHIBuffer *buffer, size_t offset, size_t size, uint32_t value)
    {
        VulkanBuffer *vulkanBuffer = reinterpret_cast<VulkanBuffer *>(buffer);

        vkCmdFillBuffer(cmd, vulkanBuffer->buffer, (VkDeviceSize)offset, (VkDeviceSize)size, value);
    }

    void VulkanCommandBuffer::copyTextureToTexture(RHITexture *src, RHITexture *dst)
    {

        VulkanTexture *vulkanTextureSrc = reinterpret_cast<VulkanTexture *>(src);
        VulkanTexture *vulkanTextureDst = reinterpret_cast<VulkanTexture *>(dst);

        VkImageCopy2 region{};
        region.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2;
        region.srcSubresource.aspectMask = vulkanTextureSrc->imageAspect;
        region.srcSubresource.layerCount = 1;

        region.dstSubresource.aspectMask = vulkanTextureDst->imageAspect;
        region.dstSubresource.layerCount = 1;
        region.extent = {vulkanTextureSrc->width, vulkanTextureSrc->height, 1};

        VkCopyImageInfo2 copyInfo{};

        copyInfo.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2;
        copyInfo.srcImage = vulkanTextureSrc->image;
        copyInfo.srcImageLayout = convertResourceStateToImageLayout(ResourceState::CopySrc);
        copyInfo.dstImage = vulkanTextureDst->image;
        copyInfo.dstImageLayout = convertResourceStateToImageLayout(ResourceState::CopyDst);
        copyInfo.regionCount = 1;
        copyInfo.pRegions = &region;

        vkCmdCopyImage2(cmd, &copyInfo);
    }

    void VulkanCommandBuffer::drawIndexedIndirect(RHIBuffer *indirectBuffer, size_t offset, uint32_t drawCount, uint32_t stride)
    {

        m_FrameStats.drawCalls += 1;

        VulkanBuffer *vulkanBuffer = reinterpret_cast<VulkanBuffer *>(indirectBuffer);

        vkCmdDrawIndexedIndirect(
            cmd,
            vulkanBuffer->buffer,
            (VkDeviceSize)offset,
            drawCount,
            stride);
    };

    void VulkanCommandBuffer::drawIndexedIndirectCount(RHIBuffer *indirectBuffer, size_t offset, RHIBuffer *countBuffer, size_t countOffset, uint32_t maxDrawCount, uint32_t stride)
    {
        m_FrameStats.drawCalls += 1;
        VulkanBuffer *vulkanIndirectBuffer = reinterpret_cast<VulkanBuffer *>(indirectBuffer);
        VulkanBuffer *vulkanCountBuffer = reinterpret_cast<VulkanBuffer *>(countBuffer);

        vkCmdDrawIndexedIndirectCount(
            cmd,
            vulkanIndirectBuffer->buffer,
            (VkDeviceSize)offset,
            vulkanCountBuffer->buffer,
            (VkDeviceSize)countOffset,
            maxDrawCount,
            stride);
    }
    void VulkanCommandBuffer::emitBarriers(std::span<const ResolvedTextureBarrier> textures, std::span<const ResolvedBufferBarrier> buffers)
    {
        m_FrameStats.barrierCalls++;
        m_FrameStats.imageBarriers += textures.size();
        m_FrameStats.bufferBarriers += buffers.size();

        std::vector<VkImageMemoryBarrier2> textureBarriers;

        textureBarriers.reserve(textures.size());

        for (auto &barrierInfo : textures)
        {
            VulkanTexture *vkTexture = static_cast<VulkanTexture *>(barrierInfo.texture);
            auto srcState = toVkState(barrierInfo.from);
            auto dstState = toVkState(barrierInfo.to);

            VkImageMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.srcStageMask = srcState.stage;
            barrier.srcAccessMask = srcState.access;
            barrier.dstStageMask = dstState.stage;
            barrier.dstAccessMask = dstState.access;

            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            barrier.image = vkTexture->image;
            barrier.oldLayout = convertResourceStateToImageLayout(barrierInfo.from);
            barrier.newLayout = convertResourceStateToImageLayout(barrierInfo.to);

            barrier.subresourceRange.aspectMask = vkTexture->imageAspect;
            barrier.subresourceRange.baseMipLevel = barrierInfo.range.baseMip;
            barrier.subresourceRange.levelCount = barrierInfo.range.mipCount;
            barrier.subresourceRange.baseArrayLayer = barrierInfo.range.baseLayer;
            barrier.subresourceRange.layerCount = barrierInfo.range.layerCount;

            textureBarriers.push_back(barrier);
        }
        std::vector<VkBufferMemoryBarrier2> bufferBarriers;
        bufferBarriers.reserve(buffers.size());

        for (auto &barrierInfo : buffers)
        {

            VulkanBuffer *vkBuffer = static_cast<VulkanBuffer *>(barrierInfo.buffer);

            auto srcState = toVkState(barrierInfo.from);
            auto dstState = toVkState(barrierInfo.to);

            VkBufferMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
            barrier.offset = 0;
            barrier.size = VK_WHOLE_SIZE;
            barrier.srcStageMask = srcState.stage;
            barrier.srcAccessMask = srcState.access;
            barrier.dstStageMask = dstState.stage;
            barrier.dstAccessMask = dstState.access;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer = vkBuffer->buffer;

            bufferBarriers.push_back(barrier);
        }

        VkDependencyInfo dependencyInfo{};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.bufferMemoryBarrierCount = uint32_t(bufferBarriers.size());
        dependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();

        dependencyInfo.imageMemoryBarrierCount = uint32_t(textureBarriers.size());
        dependencyInfo.pImageMemoryBarriers = textureBarriers.data();

        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }

    void VulkanCommandBuffer::emitCopyBufferToTexture(RHIBuffer &src, RHITexture &dst, std::span<const BufferTextureCopy> regions)
    {
        VulkanTexture &vkTexture = static_cast<VulkanTexture &>(dst);
        VulkanBuffer &vkBuffer = static_cast<VulkanBuffer &>(src);
        std::vector<VkBufferImageCopy2> bufferImageCopies;

        bufferImageCopies.reserve(regions.size());

        for (auto &region : regions)
        {
            auto resolvedRange = region.range.resolve(vkTexture.mipmapLevels, vkTexture.totalLayers);

            VkBufferImageCopy2 bufferImageCopy{};

            bufferImageCopy.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
            bufferImageCopy.bufferImageHeight = region.bufferImageHeight;
            bufferImageCopy.bufferRowLength = region.bufferRowLength;
            bufferImageCopy.bufferOffset = region.bufferOffset;
            bufferImageCopy.imageExtent = convertToVkExtent3D(region.imageExtent);
            bufferImageCopy.imageOffset = convertToVkOffset3D(region.imageOffset);
            bufferImageCopy.imageSubresource.aspectMask = vkTexture.imageAspect;
            bufferImageCopy.imageSubresource.mipLevel = resolvedRange.baseMip;
            bufferImageCopy.imageSubresource.baseArrayLayer = resolvedRange.baseLayer;
            bufferImageCopy.imageSubresource.layerCount = resolvedRange.layerCount;

            bufferImageCopies.push_back(bufferImageCopy);
        }

        VkCopyBufferToImageInfo2 copyInfo{};

        copyInfo.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
        copyInfo.dstImage = vkTexture.image;
        copyInfo.srcBuffer = vkBuffer.buffer;
        copyInfo.regionCount = static_cast<uint32_t>(bufferImageCopies.size());
        copyInfo.pRegions = bufferImageCopies.data();
        copyInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        vkCmdCopyBufferToImage2(cmd, &copyInfo);
    }
}