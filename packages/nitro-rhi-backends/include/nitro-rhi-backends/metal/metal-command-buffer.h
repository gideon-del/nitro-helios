#pragma once
#include <nitro-rhi/rhi-command-buffer.h>
#include <Metal/Metal.hpp>

namespace nitro::rhi::metal
{
    class MetalDevice;
    class MetalSwapchain;
    class MetalBuffer;
    class MetalPipeline;
    class MetalCommandBuffer : public RHICommandBuffer
    {
    public:
        MetalCommandBuffer(MetalDevice *device, MetalSwapchain *swapchain);
        ~MetalCommandBuffer() override;

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
        FrameStats getFrameStats() override;
        void resetFrameStats() override;
        void updateVertexCount(uint32_t count) override;

        MTL::CommandBuffer *commandBuffer = nullptr;
        MTL::RenderCommandEncoder *encoder = nullptr;
        MetalSwapchain *swapchain;
        MTL::RenderPassDescriptor *rpd = nullptr;

    private:
        MetalDevice *m_device;
        MetalBuffer *m_currentIndexBuffer = nullptr;
        MetalPipeline *m_pipeline = nullptr;
        FrameStats m_FrameStats;
    };
} // namespace nitro::rhi::metal
