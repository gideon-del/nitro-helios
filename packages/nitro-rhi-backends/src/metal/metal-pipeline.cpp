#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-pipeline.h>

namespace nitro::rhi::metal
{
    MetalPipeline::MetalPipeline(MetalDevice *device, const PipelineDesc &desc) : m_device(device)
    {
        NS::Error *error = nullptr;
        NS::String *vsLibPath = NS::String::string(
            desc.vertexShader.filePath.c_str(),
            NS::StringEncoding::UTF8StringEncoding);
        MTL::Library *vsLibrary = m_device->device->newLibrary(vsLibPath, &error);
        NS::String *vsName = NS::String::string(
            desc.vertexShader.name.c_str(),
            NS::StringEncoding::UTF8StringEncoding);
        MTL::Function *vertexFn = vsLibrary->newFunction(vsName);

        NS::String *fsLibPath = NS::String::string(
            desc.fragmentShader.filePath.c_str(),
            NS::StringEncoding::UTF8StringEncoding);
        MTL::Library *fsLibrary = m_device->device->newLibrary(fsLibPath, &error);
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

        pipelineState = m_device->device->newRenderPipelineState(pipeDesc, &error);

        MTL::DepthStencilDescriptor *depthDesc =
            MTL::DepthStencilDescriptor::alloc()->init();
        depthDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
        depthDesc->setDepthWriteEnabled(desc.depthTest);
        depthStencilState = m_device->device->newDepthStencilState(depthDesc);

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