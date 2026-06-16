#pragma once
#include <nitro-rhi/rhi-pipeline.h>
#include <Metal/Metal.hpp>

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
        MTL::Winding frontFace = MTL::WindingCounterClockwise;
        MTL::CullMode cullMode = MTL::CullModeBack;

    private:
        MetalDevice *m_device;
        MTL::VertexDescriptor *m_descriptor = nullptr;
    };
}