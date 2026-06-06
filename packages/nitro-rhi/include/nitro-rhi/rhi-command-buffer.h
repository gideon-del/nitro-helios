#pragma once
#include <cstdint>
#include <cstddef>

namespace nitro::rhi
{

    class RHIBuffer;
    class RHITexture;
    class RHIPipeline;
    class RHIDescriptorSet;
    class RHIRenderPass;
    struct RHIRenderPassDesc
    {
        float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        float clearDepth = 1.0f;
        bool hasDepth = true;
        RHITexture *colorTexture = nullptr;
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
    class RHICommandBuffer
    {
    public:
        virtual ~RHICommandBuffer() = default;

        virtual void beginRenderPass(const RHIRenderPassDesc &desc) = 0;
        virtual void beginRenderPass(RHIRenderPass *renderPass) = 0;
        virtual void endRenderPass() = 0;

        virtual void bindPipeline(RHIPipeline *pipeline) = 0;
        virtual void bindVertexBuffer(RHIBuffer *buffer) = 0;
        virtual void bindIndexBuffer(RHIBuffer *buffer) = 0;
        virtual void bindUniformBuffer(RHIBuffer *buffer, uint32_t binding) = 0;
        virtual void bindDescriptorSet(RHIDescriptorSet *descriptorSet, uint32_t binding) = 0;
        virtual void setPushConstant(void *data, size_t size, uint32_t binding) = 0;
        virtual void setViewPort(const RHIViewport &viewport) = 0;
        virtual void setScissor(const RHIScissor &scissor) = 0;

        virtual void draw(uint32_t vertexCount) = 0;
        virtual void drawIndexed(uint32_t indexCount) = 0;

        virtual void present() = 0;
    };

}