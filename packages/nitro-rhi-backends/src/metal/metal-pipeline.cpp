#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-pipeline.h>
#include <nitro-rhi-backends/metal/metal-utils.h>
#include <nitro-rhi-backends/metal/metal-texture.h>

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

        std::vector<MTL::Function *> shaderFunctions;

        for (const auto &shaderDesc : desc.shaders)
        {
            NS::String *libPath = NS::String::string(
                shaderDesc.filePath.c_str(),
                NS::StringEncoding::UTF8StringEncoding);
            MTL::Library *library = m_device->device->newLibrary(libPath, &error);
            checkNSError(error, "Failed to load libary");
            NS::String *vsName = NS::String::string(
                shaderDesc.name.c_str(),
                NS::StringEncoding::UTF8StringEncoding);

            shaderFunctions.push_back(library->newFunction(vsName));
            libPath->release();
            vsName->release();
            library->release();
        }

        MTL::RenderPipelineDescriptor *pipeDesc =
            MTL::RenderPipelineDescriptor::alloc()->init();

        for (int i = 0; i < desc.shaders.size(); i++)
        {
            auto &function = shaderFunctions[i];
            auto &shaderDesc = desc.shaders[i];

            if (shaderDesc.stage == ShaderStage::Fragment)
            {
                pipeDesc->setFragmentFunction(function);
            }
            if (shaderDesc.stage == ShaderStage::Vertex)
            {
                pipeDesc->setVertexFunction(function);
            }
        }

        {
            if (desc.colorAttachments.empty() && desc.hasColorAttachment)
            {
                pipeDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
            }
            else
            {
                for (int i = 0; i < desc.colorAttachments.size(); i++)
                {

                    pipeDesc->colorAttachments()->object(i)->setPixelFormat(convertToPixelFormat(
                        desc.colorAttachments[i]));
                }
            }
        }
        if (desc.depthTest)
        {

            pipeDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
        }

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
        pipeDesc->release();
        depthDesc->release();
        for (auto &function : shaderFunctions)
        {
            function->release();
        }
    }
    MetalPipeline::~MetalPipeline()
    {

        if (pipelineState)
            pipelineState->release();
        if (depthStencilState)
            depthStencilState->release();
    }
}