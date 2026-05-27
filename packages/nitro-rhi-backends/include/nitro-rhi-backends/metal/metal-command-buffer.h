#pragma once
#include <nitro-rhi/rhi-command-buffer.h>
#include <SingleHeader/MetalCpp.h>

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
        void endRenderPass() override;
        void bindPipeline(RHIPipeline *pipeline) override;
        void bindVertexBuffer(RHIBuffer *buffer) override;
        void bindIndexBuffer(RHIBuffer *buffer) override;
        void bindUniformBuffer(RHIBuffer *buffer, uint32_t binding) override;
        void bindDescriptorSet(RHIDescriptorSet *descriptorSet) override;
        void setPushConstant(void *data, size_t size, uint32_t binding) override;
        void draw(uint32_t vertexCount) override;
        void drawIndexed(uint32_t indexCount) override;
        void present() override;

        MTL::CommandBuffer *commandBuffer = nullptr;
        MTL::RenderCommandEncoder *encoder = nullptr;
        MetalSwapchain *swapchain;

    private:
        MetalDevice *m_device;
        MetalBuffer *m_currentIndexBuffer = nullptr;
        MetalPipeline *m_pipeline = nullptr;
    };
} // namespace nitro::rhi::metal
