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

    MTL::CompareFunction convertToCompareFunction(CompareOp test)
    {

        switch (test)
        {
        case CompareOp::Equal:
            return MTL::CompareFunctionEqual;
        case CompareOp::Less:
            return MTL::CompareFunctionLess;
        case CompareOp::LessOrEqual:
            return MTL::CompareFunctionLessEqual;
        case CompareOp::Greater:
            return MTL::CompareFunctionGreater;
        case CompareOp::GreaterOrEqual:
            return MTL::CompareFunctionGreaterEqual;
        case CompareOp::NotEqual:
            return MTL::CompareFunctionNotEqual;
        case CompareOp::Always:
            return MTL::CompareFunctionAlways;

        default:
            return MTL::CompareFunctionNever;
        }
    }
    MTL::BlendOperation convertBlendOp(RHIBlendDesc::BlendOp operation)
    {
        switch (operation)
        {
        case RHIBlendDesc::BlendOp::Add:
            return MTL::BlendOperationAdd;
        case RHIBlendDesc::BlendOp::Substract:
            return MTL::BlendOperationSubtract;
        default:
            return MTL::BlendOperationAdd;
        }
    }

    MTL::BlendFactor convertBlendFactor(RHIBlendDesc::BlendFactor factor)
    {
        switch (factor)
        {
        case RHIBlendDesc::BlendFactor::One:
            return MTL::BlendFactorOne;
        case RHIBlendDesc::BlendFactor::Zero:
            return MTL::BlendFactorZero;
        default:
            return MTL::BlendFactorOne;
        }
    }
    MTL::CullMode convertCullMode(PipelineDesc::CullMode cullMode)
    {
        switch (cullMode)
        {
        case PipelineDesc::CullMode::Back:
            return MTL::CullModeBack;
        case PipelineDesc::CullMode::Front:
            return MTL::CullModeFront;

        default:
            return MTL::CullModeNone;
        }
    }

    MTL::Winding convertFrontFace(PipelineDesc::FrontFace frontFace)
    {
        switch (frontFace)
        {
        case PipelineDesc::FrontFace::ClockWise:
            return MTL::WindingClockwise;

        default:
            return MTL::WindingCounterClockwise;
        }
    };

    MTL::StencilOperation convertStencilOp(RHIStencilDesc::StencilOp operation)
    {
        switch (operation)
        {
        case RHIStencilDesc::StencilOp::DECREMENT:
            return MTL::StencilOperationDecrementClamp;
        case RHIStencilDesc::StencilOp::INCREMENT:
            return MTL::StencilOperationIncrementClamp;
        case RHIStencilDesc::StencilOp::KEEP:
            return MTL::StencilOperationKeep;
        case RHIStencilDesc::StencilOp::REPLACE:
            return MTL::StencilOperationReplace;
        case RHIStencilDesc::StencilOp::ZERO:
            return MTL::StencilOperationZero;

        default:
            return MTL::StencilOperationKeep;
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
                    auto &colorAttachment = desc.colorAttachments[i];

                    pipeDesc->colorAttachments()->object(i)->setPixelFormat(convertToPixelFormat(
                        colorAttachment.format));

                    if (colorAttachment.blend.enabled)
                    {
                        pipeDesc->colorAttachments()->object(i)->setBlendingEnabled(true);
                        pipeDesc->colorAttachments()->object(i)->setRgbBlendOperation(convertBlendOp(colorAttachment.blend.operation));
                        pipeDesc->colorAttachments()->object(i)->setSourceRGBBlendFactor(convertBlendFactor(colorAttachment.blend.srcBlendFactor));
                        pipeDesc->colorAttachments()->object(i)->setDestinationRGBBlendFactor(convertBlendFactor(colorAttachment.blend.dstBlendFactor));
                    }
                }
            }
        }
        if (desc.hasDepth)
        {
            pipeDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
        }

        if (desc.stencil.enabled)
        {
            pipeDesc->setStencilAttachmentPixelFormat(MTL::PixelFormatStencil8);
        }

        if (!desc.vertexLayout.attributes.empty())
        {
            pipeDesc->setVertexDescriptor(convertToVertexDescriptor(desc.vertexLayout));
        }

        pipelineState = m_device->device->newRenderPipelineState(pipeDesc, &error);

        checkNSError(error, "Failed to load PipelineState");
        MTL::DepthStencilDescriptor *depthDesc =
            MTL::DepthStencilDescriptor::alloc()->init();
        depthDesc->setDepthCompareFunction(convertToCompareFunction(desc.depthTest));
        depthDesc->setDepthWriteEnabled(desc.depthWrite);
        MTL::StencilDescriptor *frontStencilDesc = nullptr;
        MTL::StencilDescriptor *backStencilDesc = nullptr;
        if (desc.stencil.enabled)
        {
            frontStencilDesc = MTL::StencilDescriptor::alloc()->init();

            frontStencilDesc->setDepthFailureOperation(convertStencilOp(desc.stencil.front.depthFailOp));
            frontStencilDesc->setStencilFailureOperation(convertStencilOp(desc.stencil.front.failOp));
            frontStencilDesc->setDepthStencilPassOperation(convertStencilOp(desc.stencil.front.passOp));

            frontStencilDesc->setReadMask(desc.stencil.front.compareMask);
            frontStencilDesc->setWriteMask(desc.stencil.front.writeMask);
            frontStencilDesc->setStencilCompareFunction(convertToCompareFunction(desc.stencil.front.compareOp));

            depthDesc->setFrontFaceStencil(frontStencilDesc);

            backStencilDesc = MTL::StencilDescriptor::alloc()->init();

            backStencilDesc->setDepthFailureOperation(convertStencilOp(desc.stencil.back.depthFailOp));
            backStencilDesc->setStencilFailureOperation(convertStencilOp(desc.stencil.back.failOp));
            backStencilDesc->setDepthStencilPassOperation(convertStencilOp(desc.stencil.back.passOp));

            backStencilDesc->setReadMask(desc.stencil.back.compareMask);
            backStencilDesc->setWriteMask(desc.stencil.back.writeMask);
            backStencilDesc->setStencilCompareFunction(convertToCompareFunction(desc.stencil.back.compareOp));

            depthDesc->setBackFaceStencil(backStencilDesc);
        }
        depthStencilState = m_device->device->newDepthStencilState(depthDesc);
        topology = convertToPrimitive(desc.topology);
        frontFace = convertFrontFace(desc.frontFace);
        cullMode = convertCullMode(desc.cullMode);
        pipeDesc->release();
        depthDesc->release();

        if (frontStencilDesc)
        {
            frontStencilDesc->release();
        }
        if (backStencilDesc)
        {
            backStencilDesc->release();
        }
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