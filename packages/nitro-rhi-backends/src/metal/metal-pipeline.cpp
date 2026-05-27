#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-pipeline.h>
#include <nitro-rhi-backends/metal/metal-utils.h>

namespace nitro::rhi::metal
{

    MTL::VertexFormat convertToVertexFormat(RHIVertexLayout::Attributes::Format format)
    {
        switch (format)
        {
        case RHIVertexLayout::Attributes::Format::Float:
            return MTL::VertexFormatFloat;
        case RHIVertexLayout::Attributes::Format::Float2:
            return MTL::VertexFormatFloat2;
        case RHIVertexLayout::Attributes::Format::Float3:
            return MTL::VertexFormatFloat3;
        case RHIVertexLayout::Attributes::Format::Float4:
            return MTL::VertexFormatFloat4;
        }

        return MTL::VertexFormatFloat;
    }
    MTL::VertexDescriptor *convertToVertexDescriptor(const RHIVertexLayout &vertexLayout)
    {
        MTL::VertexDescriptor *descriptor = MTL::VertexDescriptor::alloc()->init();

        for (int i = 0; i < vertexLayout.attributes.size(); i++)
        {
            descriptor->attributes()->object(NS::UInteger(i))->setFormat(convertToVertexFormat(vertexLayout.attributes[i].format));
            descriptor->attributes()->object(NS::UInteger(i))->setOffset(NS::UInteger(vertexLayout.attributes[i].offset));
            descriptor->attributes()->object(NS::UInteger(i))->setBufferIndex(NS::UInteger(vertexLayout.binding));
        }

        descriptor->layouts()->object(NS::UInteger(vertexLayout.binding))->setStride(NS::UInteger(vertexLayout.stride));
        descriptor->layouts()->object(NS::UInteger(vertexLayout.binding))->setStepFunction(MTL::VertexStepFunctionPerVertex);

        return descriptor;
    }
    MTL::PrimitiveType convertToPrimitive(PipelineTopology topology)
    {
        switch (topology)
        {
        case PipelineTopology::TriangleList:
            return MTL::PrimitiveTypeTriangle;
        case PipelineTopology::LineList:
            return MTL::PrimitiveTypeLine;
        case PipelineTopology::PointList:
            return MTL::PrimitiveTypePoint;
        default:
            return MTL::PrimitiveTypeTriangle;
        }
    }
    MetalPipeline::MetalPipeline(MetalDevice *device, const PipelineDesc &desc) : m_device(device)
    {
        NS::Error *error = nullptr;
        NS::String *vsLibPath = NS::String::string(
            desc.vertexShader.filePath.c_str(),
            NS::StringEncoding::UTF8StringEncoding);
        MTL::Library *vsLibrary = m_device->device->newLibrary(vsLibPath, &error);

        checkNSError(error, "Failed to load VSLibary");
        NS::String *vsName = NS::String::string(
            desc.vertexShader.name.c_str(),
            NS::StringEncoding::UTF8StringEncoding);
        MTL::Function *vertexFn = vsLibrary->newFunction(vsName);

        NS::String *fsLibPath = NS::String::string(
            desc.fragmentShader.filePath.c_str(),
            NS::StringEncoding::UTF8StringEncoding);
        MTL::Library *fsLibrary = m_device->device->newLibrary(fsLibPath, &error);
        checkNSError(error, "Failed to load FsLibary");
        NS::String *fsName = NS::String::string(
            desc.fragmentShader.name.c_str(),
            NS::StringEncoding::UTF8StringEncoding);
        MTL::Function *fragmentFn = fsLibrary->newFunction(fsName);

        MTL::RenderPipelineDescriptor *pipeDesc =
            MTL::RenderPipelineDescriptor::alloc()->init();

        pipeDesc->setVertexFunction(vertexFn);
        pipeDesc->setFragmentFunction(fragmentFn);
        pipeDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
        pipeDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

        if (!desc.vertexLayout.attributes.empty())
        {
            pipeDesc->setVertexDescriptor(convertToVertexDescriptor(desc.vertexLayout));
        }

        pipelineState = m_device->device->newRenderPipelineState(pipeDesc, &error);
        checkNSError(error, "Failed to load PipelineState");
        MTL::DepthStencilDescriptor *depthDesc =
            MTL::DepthStencilDescriptor::alloc()->init();
        depthDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
        depthDesc->setDepthWriteEnabled(desc.depthTest);
        depthStencilState = m_device->device->newDepthStencilState(depthDesc);
        topology = convertToPrimitive(desc.topology);
        vertexFn->release();
        fragmentFn->release();
        vsLibrary->release();
        fsLibrary->release();
        vsName->release();
        fsName->release();
        pipeDesc->release();
        depthDesc->release();
    }
    MetalPipeline::~MetalPipeline()
    {

        if (pipelineState)
            pipelineState->release();
        if (depthStencilState)
            depthStencilState->release();
    }
}