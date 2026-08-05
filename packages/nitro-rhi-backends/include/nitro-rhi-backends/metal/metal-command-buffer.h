#pragma once
#include <nitro-rhi/rhi-command-buffer.h>
#include <Metal/Metal.hpp>

namespace nitro::rhi::metal
{
    class MetalDevice;
    class MetalSwapchain;
    class MetalBuffer;
    class MetalPipeline;
    class MetalComputePipeline;
    class MetalCommandBuffer : public RHICommandBuffer
    {
    public:
        MetalCommandBuffer(MetalDevice *device, MetalSwapchain *swapchain);
        MetalCommandBuffer(MetalDevice *device);
        ~MetalCommandBuffer() override;

        void beginRenderPass(const RHIRenderPassDesc &desc) override;
        void beginRenderPass(RHIRenderPass *renderPass) override;
        void endRenderPass() override;
        void bindPipeline(RHIPipeline *pipeline) override;
        void bindComputePipeline(RHIComputePipeline *pipeline) override;
        void bindVertexBuffer(RHIBuffer *buffer) override;
        void bindIndexBuffer(RHIBuffer *buffer) override;
        void bindUniformBuffer(RHIBuffer *buffer, uint32_t binding) override;
        void bindDescriptorSet(RHIDescriptorSet *descriptorSet, uint32_t binding) override;
        void bindComputeDescriptorSet(RHIDescriptorSet *descriptorSet, uint32_t binding) override;
        ;
        void setPushConstant(void *data, size_t size, uint32_t binding, bool isCompute = false) override;
        void setViewPort(const RHIViewport &viewport) override;
        void setScissor(const RHIScissor &scissor) override;
        void setStencilReference(uint32_t reference) override;
        void bufferBarrier(const BufferBarrier &barrier) override;
        void draw(uint32_t vertexCount, uint32_t instanceCount = 1) override;
        void drawIndirect(RHIBuffer *indirectBuffer, size_t offset = 0) override;
        void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1) override;
        void dispatch(uint32_t x, uint32_t y, uint32_t z) override;
        void dispatchIndirect(RHIBuffer *indirectBuffer, size_t offset = 0) override;
        void submit() override;
        void present() override;
        FrameStats getFrameStats() override;
        void resetFrameStats() override;
        void updateVertexCount(uint32_t count) override;
        void generateMipmaps(RHITexture *texture) override;
        void textureBarrier(const TextureBarrier &barrier) override;
        void copyTextureToBuffer(RHITexture *texture, RHIBuffer *buffer) override;
        void fillBuffer(RHIBuffer *buffer, size_t offset, size_t size, uint32_t value) override;
        void endEncoders();
        MTL::CommandBuffer *commandBuffer = nullptr;
        MTL::RenderCommandEncoder *encoder = nullptr;
        MetalSwapchain *swapchain;
        MTL::RenderPassDescriptor *rpd = nullptr;

    private:
        MetalDevice *m_device;
        MetalBuffer *m_currentIndexBuffer = nullptr;
        MetalPipeline *m_pipeline = nullptr;
        FrameStats m_FrameStats;
        MTL::ComputeCommandEncoder *m_computeEncoder = nullptr;
        MetalComputePipeline *m_computePipeline = nullptr;
    };
} // namespace nitro::rhi::metal
