#pragma once
#include <nitro-rhi/rhi-command-buffer.h>
#include <vulkan/vulkan.h>

namespace nitro::rhi::vulkan
{
    class VulkanDevice;
    class VulkanSwapchain;
    class VulkanPipeline;
    class VulkanRenderPass;
    class VulkanCommandBuffer : public RHICommandBuffer
    {
    public:
        VulkanCommandBuffer(VulkanDevice *device, VulkanSwapchain *swapchain,
                            VkCommandBuffer cmd, uint32_t frameIdx);
        ~VulkanCommandBuffer() override;

        void setImageIndex(uint32_t imageIdx) { m_imageIdx = imageIdx; }

        void beginRenderPass(const RHIRenderPassDesc &desc) override;
        void beginRenderPass(RHIRenderPass *renderPass) override;
        void endRenderPass() override;
        void bindPipeline(RHIPipeline *pipeline) override;
        void bindVertexBuffer(RHIBuffer *buffer) override;
        void bindIndexBuffer(RHIBuffer *buffer) override;
        void bindUniformBuffer(RHIBuffer *buffer, uint32_t binding) override;
        void bindDescriptorSet(RHIDescriptorSet *descriptorSet, uint32_t binding) override;
        void setPushConstant(void *data, size_t size, uint32_t binding) override;
        void setViewPort(const RHIViewport &viewport) override;
        void setScissor(const RHIScissor &scissor) override;
        void draw(uint32_t vertexCount) override;
        void drawIndexed(uint32_t indexCount) override;
        void present() override;

        VkCommandBuffer cmd;
        VkFence inFlight;
        VkSemaphore imageAvailable;
        VulkanSwapchain *swapchain;

    private:
        VulkanDevice *m_device;
        uint32_t m_imageIdx = 0;
        uint32_t m_frameIdx;
        VulkanPipeline *m_pipeline = nullptr;
        VulkanRenderPass *m_activeRenderPass = nullptr;
    };
} // namespace nitro::rhi::vulkan
