#pragma once
#include "rhi-buffer.h"
#include "rhi-command-buffer.h"
#include "rhi-texture.h"
#include "rhi-pipeline.h"
#include "rhi-surface.h"
#include "rhi-swapchain.h"
#include "rhi-descriptor-layout.h"
#include "rhi-descriptor-set.h"
#include "rhi-render-pass.h"
#include "rhi-timer.h"
namespace nitro::rhi
{

    class RHIDevice
    {
    public:
        virtual ~RHIDevice() = default;

        virtual RHISwapchain *createSwapchain(RHISurface *surface) = 0;

        virtual RHIDescriptorLayout *createDescriptorLayout(const std::vector<RHIDescriptorBinding> bindings) = 0;
        virtual RHIDescriptorSet *createDescriptorSet(RHIDescriptorLayout *layout) = 0;

        virtual RHITimer *createTimer() = 0;

        virtual RHIRenderPass *createRenderPass(const RenderPassDesc &desc) = 0;

        virtual RHIBuffer *createBuffer(const BufferDesc &desc) = 0;
        virtual void destroyBuffer(RHIBuffer *buffer) = 0;

        virtual RHITexture *createTexture(const TextureDesc &desc) = 0;
        virtual void destroyTexture(RHITexture *texture) = 0;

        virtual RHIPipeline *createPipeline(const PipelineDesc &desc) = 0;
        virtual void destroyPipeline(RHIPipeline *pipeline) = 0;

        virtual uint32_t getCurrentFrameIndex() const = 0;
        virtual RHICommandBuffer *beginFrame() = 0;
        virtual void endFrame(RHICommandBuffer *cmd) = 0;
        virtual void beginImGuiFrame() = 0;
        virtual void endImGuiFrame() = 0;
        virtual void drawImGui(RHICommandBuffer *cmd) = 0;
    };
}