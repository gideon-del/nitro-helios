#pragma once
#include "nitro-rhi/rhi-command-buffer.h"

namespace nitro::tests
{
    using namespace rhi;
    class NullCommandBuffer : public RHICommandBuffer
    {
    public:
        ~NullCommandBuffer() override {};

        void beginRenderPass(const RHIRenderPassDesc &desc) override {}
        void beginRenderPass(RHIRenderPass *renderPass) override {}
        void endRenderPass() override {}

        void bindPipeline(RHIPipeline *pipeline) override {}
        void bindComputePipeline(RHIComputePipeline *pipeline) override {}
        void bindVertexBuffer(RHIBuffer *buffer) override {}
        void bindIndexBuffer(RHIBuffer *buffer) override {}
        void bindUniformBuffer(RHIBuffer *buffer, uint32_t binding) override {}
        void bindDescriptorSet(RHIDescriptorSet *descriptorSet, uint32_t binding) override {}
        void bindComputeDescriptorSet(RHIDescriptorSet *descriptorSet, uint32_t binding) override {}
        void setPushConstant(void *data, size_t size, uint32_t binding, bool isCompute = false) override {}
        void setViewPort(const RHIViewport &viewport) override {}
        void setScissor(const RHIScissor &scissor) override {}
        void setStencilReference(uint32_t reference) override {}
        void generateMipmaps(RHITexture *texture) override {}

        void fillBuffer(RHIBuffer *buffer, size_t offset, size_t size, uint32_t value) override {}
        void draw(uint32_t vertexCount, uint32_t instanceCount = 1) override {}

        void drawIndirect(RHIBuffer *indirectBuffer, size_t offset = 0) override {}
        void drawIndexedIndirect(RHIBuffer *indirectBuffer, size_t offset, uint32_t drawCount, uint32_t stride) override {}
        void drawIndexedIndirectCount(RHIBuffer *indirectBuffer, size_t offset, RHIBuffer *countBuffer, size_t countOffset, uint32_t maxDrawCount, uint32_t stride) override {}
        void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1) override {}

        void copyTextureToBuffer(RHITexture *texture, RHIBuffer *buffer) override {}
        void copyTextureToTexture(RHITexture *src, RHITexture *dst) override {}
        FrameStats getFrameStats() override
        {
            return {};
        }
        void resetFrameStats() override {}
        void updateVertexCount(uint32_t count) override {}
        void submit() override {}
        void present() override {}
        void dispatch(uint32_t x, uint32_t y, uint32_t z) override {}
        void dispatchIndirect(RHIBuffer *indirectBuffer, size_t offset = 0) override {}

    protected:
        void emitBarriers(std::span<const ResolvedTextureBarrier> textures, std::span<const ResolvedBufferBarrier> buffers) override {}
        void emitCopyBufferToTexture(RHIBuffer &src, RHITexture &dst, std::span<const BufferTextureCopy> regions) override {};
    };
} // namespace nitro::tests
