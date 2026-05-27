#pragma once
#include <nitro-rhi/rhi-pipeline.h>
#include <SingleHeader/MetalCpp.h>

namespace nitro::rhi::metal
{
    class MetalDevice;
    class MetalPipeline : public RHIPipeline
    {
    public:
        MetalPipeline(MetalDevice *device, const PipelineDesc &desc);
        ~MetalPipeline() override;

        MTL::RenderPipelineState *pipelineState;
        MTL::DepthStencilState *depthStencilState;
        MTL::PrimitiveType topology = MTL::PrimitiveTypeTriangle;

    private:
        MetalDevice *m_device;
        MTL::VertexDescriptor *m_descriptor = nullptr;
    };
}