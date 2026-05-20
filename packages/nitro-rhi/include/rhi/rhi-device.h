#pragma once
#include "rhi-buffer.h"
#include "rhi-command-buffer.h"
#include "rhi-texture.h"
#include "rhi-pipeline.h"
namespace nitro::rhi
{

    class RHIDevice
    {
    public:
        virtual ~RHIDevice() = default;
        virtual RHIBuffer *createBuffer(const BufferDesc &desc) = 0;
        virtual void destroyBuffer(RHIBuffer *buffer) = 0;

        virtual RHITexture *createTexture(const TextureDesc &desc) = 0;
        virtual void destroyTexture(RHITexture *texture) = 0;

        virtual RHIPipeline *createPipeline(const PipelineDesc &desc) = 0;
        virtual void destroyPipeline(RHIPipeline *pipeline) = 0;

        virtual RHICommandBuffer *beginFrame() = 0;
        virtual void endFrame(RHICommandBuffer *cmd) = 0;
    };
}