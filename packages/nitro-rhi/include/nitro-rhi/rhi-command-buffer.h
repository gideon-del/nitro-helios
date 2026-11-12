#pragma once
#include <cstdint>
#include <cstddef>
#include <span>
#include "rhi-texture.h"
#include "rhi-buffer.h"
#include "nitro-core/types/size.h"

namespace nitro::rhi
{
    struct FrameStats
    {
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
        uint32_t vertices = 0;
        uint32_t barrierCalls = 0;
        uint32_t imageBarriers = 0;
        uint32_t bufferBarriers = 0;
    };

    class RHIPipeline;
    class RHIComputePipeline;
    class RHIDescriptorSet;
    class RHIRenderPass;

    struct RHIRenderPassDesc
    {
        float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        float clearDepth = 1.0f;
        bool hasDepth = true;
        RHITexture *colorTexture = nullptr;
        float depthBiasConstant = 0.0f;
        float depthBiasSlopScale = 0.0f;
        float depthBiasClamp = 0.0f;
    };

    struct RHIViewport
    {
        float x = 0.0f,
              y = 0.0f,
              width,
              height,
              minDepth = 0.0f,
              maxDepth = 1.0f;
    };
    struct RHIScissor
    {
        int32_t x = 0;
        int32_t y = 0;
        uint32_t width;
        uint32_t height;
    };

    struct DrawIndirectArgs
    {
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t firstInstance;
    };

    struct BufferTextureCopy
    {

        uint64_t bufferOffset = 0;
        uint32_t bufferRowLength = 0;
        uint32_t bufferImageHeight = 0;

        TextureSubresource range{.baseMip = 0, .mipCount = 1};
        Offset3D imageOffset = {};
        Extent3D imageExtent = {};
    };
    struct DrawIndexedIndirectArgs
    {
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        int vertexOffset;
        uint32_t firstInstance;
    };

    struct DispatchIndirectArgs
    {
        uint32_t x;
        uint32_t y;
        uint32_t z;
    };

    struct ResolvedTextureBarrier
    {
        RHITexture *texture;
        ResourceState from;
        ResourceState to;
        TextureSubresource range;
    };

    struct ResolvedBufferBarrier
    {
        RHIBuffer *buffer;
        ResourceState from;
        ResourceState to;
    };

    inline bool isWriteState(ResourceState s)
    {
        switch (s)
        {
        case ResourceState::ShaderWrite:
        case ResourceState::RenderTarget:
        case ResourceState::DepthWrite:
        case ResourceState::CopyDst:
            return true;
        default:
            return false;
        }
    }
    inline bool needsBarrier(ResourceState from, ResourceState to)
    {
        return from != to || isWriteState(to);
    }
    class RHICommandBuffer
    {
    public:
        virtual ~RHICommandBuffer() = default;

        virtual void beginRenderPass(const RHIRenderPassDesc &desc) = 0;
        virtual void beginRenderPass(RHIRenderPass *renderPass) = 0;
        virtual void endRenderPass() = 0;

        virtual void bindPipeline(RHIPipeline *pipeline) = 0;
        virtual void bindComputePipeline(RHIComputePipeline *pipeline) = 0;
        virtual void bindVertexBuffer(RHIBuffer *buffer) = 0;
        virtual void bindIndexBuffer(RHIBuffer *buffer) = 0;
        virtual void bindUniformBuffer(RHIBuffer *buffer, uint32_t binding) = 0;
        virtual void bindDescriptorSet(RHIDescriptorSet *descriptorSet, uint32_t binding) = 0;
        virtual void bindComputeDescriptorSet(RHIDescriptorSet *descriptorSet, uint32_t binding) = 0;
        virtual void setPushConstant(void *data, size_t size, uint32_t binding, bool isCompute = false) = 0;
        virtual void setViewPort(const RHIViewport &viewport) = 0;
        virtual void setScissor(const RHIScissor &scissor) = 0;
        virtual void setStencilReference(uint32_t reference) = 0;
        virtual void generateMipmaps(RHITexture *texture) = 0;
        virtual void fillBuffer(RHIBuffer *buffer, size_t offset, size_t size, uint32_t value) = 0;
        virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1) = 0;

        virtual void drawIndirect(RHIBuffer *indirectBuffer, size_t offset = 0) = 0;
        virtual void drawIndexedIndirect(RHIBuffer *indirectBuffer, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void drawIndexedIndirectCount(RHIBuffer *indirectBuffer, size_t offset, RHIBuffer *countBuffer, size_t countOffset, uint32_t maxDrawCount, uint32_t stride) = 0;
        virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1) = 0;

        virtual void copyTextureToBuffer(RHITexture *texture, RHIBuffer *buffer) = 0;
        virtual void copyTextureToTexture(RHITexture *src, RHITexture *dst) = 0;
        virtual FrameStats getFrameStats() = 0;
        virtual void resetFrameStats() = 0;
        virtual void updateVertexCount(uint32_t count) = 0;
        virtual void submit() = 0;
        virtual void present() = 0;
        virtual void dispatch(uint32_t x, uint32_t y, uint32_t z) = 0;
        virtual void dispatchIndirect(RHIBuffer *indirectBuffer, size_t offset = 0) = 0;
        void copyBufferToTexture(RHIBuffer &src, RHITexture &dst, std::span<const BufferTextureCopy> regions)
        {
            auto extent = dst.extent();

            for (auto &region : regions)
            {
                auto state = dst.state();
                auto resolvedRange = region.range.resolve(state.mipCount(), state.layerCount());
                assert(resolvedRange.mipCount == 1 && "Only one mip per region");
                assert(state.isRangeIn(ResourceState::CopyDst, resolvedRange.baseMip, resolvedRange.mipCount, resolvedRange.baseLayer, resolvedRange.layerCount) && "barrier to CopyDst first");
                assert(region.bufferRowLength == 0 || region.bufferRowLength >= region.imageExtent.width);
                assert(region.bufferImageHeight == 0 || region.bufferImageHeight >= region.imageExtent.height);
                assert(region.imageExtent.width && region.imageExtent.height && region.imageExtent.depth && "Must be greater than 0");

                auto mipWidth = std::max(1u, extent.width >> resolvedRange.baseMip);
                auto mipHeight = std::max(1u, extent.height >> resolvedRange.baseMip);

                assert(region.imageOffset.x >= 0 && region.imageOffset.y >= 0);
                assert(region.imageExtent.width + region.imageOffset.x <= mipWidth && "Out of bound copy");
                assert(region.imageExtent.height + region.imageOffset.y <= mipHeight && "Out of bound copy");

                const uint32_t bpt = getImageFormatSize(dst.format());
                const uint32_t rowLen = region.bufferRowLength ? region.bufferRowLength : region.imageExtent.width;
                const uint32_t rows = region.bufferImageHeight ? region.bufferImageHeight : region.imageExtent.height;

                const uint64_t bytesPerRow = uint64_t(rowLen) * bpt;
                const uint64_t bytesPerImage = bytesPerRow * rows;

                auto resolved = region.range.resolve(state.mipCount(), state.layerCount());
                uint64_t lastByte = region.bufferOffset + (resolved.layerCount - 1) * bytesPerImage + (region.imageExtent.height - 1) * bytesPerRow + uint64_t(region.imageExtent.width) * bpt;
                assert(lastByte <= src.getSize() && "copy reads past the end of the staging buffer");
            }
            emitCopyBufferToTexture(src, dst, regions);
        };
        void barrier(std::span<const TextureBarrier> textures, std::span<const BufferBarrier> buffers = {})
        {

            std::vector<ResolvedTextureBarrier> textureResolvedBarriers;

            for (auto &barrier : textures)
            {
                transition(barrier, textureResolvedBarriers);
            }

            std::vector<ResolvedBufferBarrier> bufferResolvedBarriers;

            for (auto &barrier : buffers)
            {
                transition(barrier, bufferResolvedBarriers);
            }

            if (textureResolvedBarriers.empty() && bufferResolvedBarriers.empty())
                return;
            emitBarriers(textureResolvedBarriers, bufferResolvedBarriers);
        };
        void discard(RHITexture &texture) { mutableState(texture).updateAll(ResourceState::Undefined); }

    protected:
        static TextureState &mutableState(RHITexture &texture) { return texture.m_resourceState; }
        static ResourceState &mutableState(RHIBuffer &buffer) { return buffer.m_state; }
        virtual void emitBarriers(std::span<const ResolvedTextureBarrier> textures, std::span<const ResolvedBufferBarrier> buffers) = 0;
        virtual void emitCopyBufferToTexture(RHIBuffer &src, RHITexture &dst, std::span<const BufferTextureCopy> regions) = 0;

    private:
        void transition(const TextureBarrier &barrier, std::vector<ResolvedTextureBarrier> &batch)
        {
            if (!barrier.texture)
                return;

            auto &state = mutableState(*barrier.texture);

            auto resolvedRange = barrier.range.resolve(state.mipCount(), state.layerCount());

            if (state.isUniform())
            {
                auto from = state.get(0);
                if (!needsBarrier(from, barrier.to))
                    return;
                batch.push_back({barrier.texture, from, barrier.to, resolvedRange});
                if (barrier.range.isAll())
                    state.updateAll(barrier.to);
                else
                    state.updateRange(barrier.to, resolvedRange.baseMip, resolvedRange.mipCount, resolvedRange.baseLayer, resolvedRange.layerCount);
                return;
            }

            for (uint32_t layer = resolvedRange.baseLayer; layer < resolvedRange.baseLayer + resolvedRange.layerCount; ++layer)
            {

                uint32_t runStart = resolvedRange.baseMip;
                ResourceState runState = state.get(state.subIndex(runStart, layer));

                auto flush = [&](uint32_t end)
                {
                    if (needsBarrier(runState, barrier.to))
                    {

                        TextureSubresource newRange{};
                        newRange.baseMip = runStart;
                        newRange.baseLayer = layer;
                        newRange.mipCount = end - runStart;
                        newRange.layerCount = 1;

                        batch.push_back({.texture = barrier.texture,
                                         .from = runState,
                                         .to = barrier.to,
                                         .range = newRange});
                    }
                };

                for (uint32_t mip = resolvedRange.baseMip; mip < resolvedRange.baseMip + resolvedRange.mipCount; mip++)
                {
                    auto subState = state.get(state.subIndex(mip, layer));

                    if (subState != runState)
                    {
                        flush(mip);
                        runState = subState;
                        runStart = mip;
                    }
                }

                flush(resolvedRange.baseMip + resolvedRange.mipCount);
            }
            state.updateRange(barrier.to, resolvedRange.baseMip, resolvedRange.mipCount, resolvedRange.baseLayer, resolvedRange.layerCount);
        }

        void transition(const BufferBarrier &barrier, std::vector<ResolvedBufferBarrier> &batch)
        {

            if (!barrier.buffer)
                return;
            auto &state = mutableState(*barrier.buffer);

            if (needsBarrier(state, barrier.to))
            {
                batch.push_back({.buffer = barrier.buffer,
                                 .from = state,
                                 .to = barrier.to});
                state = barrier.to;
            }
        }
    };

}