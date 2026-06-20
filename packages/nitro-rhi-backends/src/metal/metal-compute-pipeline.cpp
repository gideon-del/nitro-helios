#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-compute-pipeline.h>
#include <nitro-rhi-backends/metal/metal-utils.h>

namespace nitro::rhi::metal
{
    MetalComputePipeline::MetalComputePipeline(MetalDevice *device, const ComputePipelineDesc &desc)
        : m_device(device),
          threadGroupSizeX(desc.threadGroupSizeX),
          threadGroupSizeY(desc.threadGroupSizeY),
          threadGroupSizeZ(desc.threadGroupSizeZ)
    {
        NS::Error *error = nullptr;

        NS::String *libPath = NS::String::string(
            desc.shader.filePath.c_str(),
            NS::StringEncoding::UTF8StringEncoding);
        MTL::Library *library = m_device->device->newLibrary(libPath, &error);
        checkNSError(error, "Failed to load library");

        NS::String *fnName = NS::String::string(
            desc.shader.name.c_str(),
            NS::StringEncoding::UTF8StringEncoding);
        MTL::Function *computeFunction = library->newFunction(fnName);

        pipelineState = m_device->device->newComputePipelineState(computeFunction, &error);
        checkNSError(error, "Failed to create Compute PipelineState");

        libPath->release();
        fnName->release();
        library->release();
        computeFunction->release();
    }

    MetalComputePipeline::~MetalComputePipeline()
    {
        if (pipelineState)
            pipelineState->release();
    }
}