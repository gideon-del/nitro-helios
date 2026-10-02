#include <nitro-renderer/passes/tiled-deffered-compute-pass.h>

namespace nitro::renderer
{
    TiledLightingComputePass::TiledLightingComputePass(
        std::shared_ptr<rhi::RHIDevice> device,
        uint32_t width,
        uint32_t height,
        uint32_t maxPointLights,
        std::string shaderDir,
        bool isMetal) : m_device(device), m_width(width), m_height(height), m_maxPointLights(maxPointLights)
    {
        std::vector<rhi::RHIDescriptorBinding> bindings{
            {rhi::RHIDescriptorBinding::Type::UniformBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             2},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             3},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             4},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             5},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             6},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             7},
        };

        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = false;
        computePipelineDesc.threadGroupSizeX = 16;
        computePipelineDesc.threadGroupSizeY = 16;
        computePipelineDesc.threadGroupSizeZ = 1;
        computePipelineDesc.layouts = {m_descriptorLayout};

        std::string shaderPath = shaderDir + "/tiled-deferred-culling/tiled-deferred-culling";

        computePipelineDesc.shader.stage = ShaderStage::Compute;
        if (isMetal)
        {
            computePipelineDesc.shader.name = "comp";
            computePipelineDesc.shader.filePath = shaderPath + ".metallib";
        }
        else
        {
            computePipelineDesc.shader.name = "main";
            computePipelineDesc.shader.filePath = shaderPath + ".comp.spv";
        }

        m_computePipeline = m_device->createComputePipeline(computePipelineDesc);

        m_tileSizeX = static_cast<uint32_t>(ceil(float(m_width) / float(TiledLightingComputePass::c_TILE_GROUP_SIZE)));
        m_tileSizeY = static_cast<uint32_t>(ceil(float(m_height) / float(TiledLightingComputePass::c_TILE_GROUP_SIZE)));
        rhi::RenderPassDesc renderPassDesc;
        renderPassDesc.width = m_width;
        renderPassDesc.height = m_height;

        m_resources.create(g_MAX_FRAMES_IN_FLIGHT,
                           [&](uint32_t frameIdx)
                           {
                               TileLightingComputeResource resource;

                               rhi::BufferDesc uboDesc;
                               uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                               uboDesc.usage = rhi::BufferDesc::Usage::Uniform;
                               uboDesc.size = sizeof(TiledCameraUBO);
                               resource.cameraUniformBuffer = m_device->createBuffer(uboDesc);

                               resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

                               return resource;
                           });
    }

    TiledLightingComputePass::~TiledLightingComputePass()
    {

        for (auto &resource : m_resources)
        {

            m_device->destroyDescriptorSet(resource.descriptorSet);
            m_device->destroyBuffer(resource.cameraUniformBuffer);
        }

        m_device->destroyComputePipeline(m_computePipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    };

    void TiledLightingComputePass::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
        m_tileSizeX = static_cast<uint32_t>(ceil(float(m_width) / float(TiledLightingComputePass::c_TILE_GROUP_SIZE)));
        m_tileSizeY = static_cast<uint32_t>(ceil(float(m_height) / float(TiledLightingComputePass::c_TILE_GROUP_SIZE)));
    }

    bool TiledLightingComputePass::isFrameResourceStale(const RGResources &rgResources, const TileLightingComputeRGResource &rgResourceIds, rhi::RHIBuffer *pointLightBuffer, TileLightingComputeResource &resource)
    {
        auto frameIdx = m_device->getCurrentFrameIndex();
        auto tileIndicesBuffer = rgResources.getBuffer(rgResourceIds.tileLightIndices, frameIdx);
        auto tileCountBuffer = rgResources.getBuffer(rgResourceIds.tileLightCount, frameIdx);
        auto tileDebugBuffer = rgResources.getBuffer(rgResourceIds.tileLightDebug, frameIdx);
        auto depthTexture = rgResources.getTexture(rgResourceIds.depthTexture);

        return resource.lastDepthTexture != depthTexture || resource.lastPointLightBuffer != pointLightBuffer || resource.lastTileLightCountBuffer != tileCountBuffer || resource.lastTileLightIndicesBuffer != tileIndicesBuffer || resource.latTileLightDebugBuffer != tileDebugBuffer;
    }

    void TiledLightingComputePass::bindFrameResource(const RGResources &rgResources, const TileLightingComputeRGResource &rgResourceIds, rhi::RHIBuffer *pointLightBuffer, TileLightingComputeResource &resource)
    {
        auto frameIdx = m_device->getCurrentFrameIndex();
        auto tileIndicesBuffer = rgResources.getBuffer(rgResourceIds.tileLightIndices, frameIdx);
        auto tileCountBuffer = rgResources.getBuffer(rgResourceIds.tileLightCount, frameIdx);
        auto tileDebugBuffer = rgResources.getBuffer(rgResourceIds.tileLightDebug, frameIdx);
        auto depthTexture = rgResources.getTexture(rgResourceIds.depthTexture);

        resource.lastPointLightBuffer = pointLightBuffer;
        resource.lastDepthTexture = depthTexture;
        resource.lastTileLightCountBuffer = tileCountBuffer;
        resource.lastTileLightIndicesBuffer = tileIndicesBuffer;
        resource.latTileLightDebugBuffer = tileDebugBuffer;

        resource.descriptorSet->writeBuffer(resource.cameraUniformBuffer, 2);
        rhi::TextureBinding textureBinding;
        textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
        textureBinding.texture = depthTexture;
        resource.descriptorSet->writeTexture(textureBinding, 3, rhi::ImageLayout::ShaderReadOnly);
        resource.descriptorSet->writeBuffer(pointLightBuffer, 4);
        resource.descriptorSet->writeBuffer(tileCountBuffer, 5);
        resource.descriptorSet->writeBuffer(tileIndicesBuffer, 6);
        resource.descriptorSet->writeBuffer(tileDebugBuffer, 7);

        resource.descriptorSet->commit();
    }
    void TiledLightingComputePass::execute(rhi::RHICommandBuffer *cmd, const RGResources &rgResources, const TileLightingComputeRGResource &rgResourceIds, Scene &scene, TiledCameraUBO cameraUBO)
    {

        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());
        auto pointLightBuffer = scene.lightManager()->getPointLightBuffer();
        if (isFrameResourceStale(rgResources, rgResourceIds, pointLightBuffer, resource))
        {
            bindFrameResource(rgResources, rgResourceIds, pointLightBuffer, resource);
        }
        cameraUBO.numTilesX = m_tileSizeX;
        cameraUBO.numTilesY = m_tileSizeY;
        resource.cameraUniformBuffer->upload(&cameraUBO, sizeof(TiledCameraUBO));

        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(resource.descriptorSet, 0);
        cmd->dispatch(m_tileSizeX, m_tileSizeY, 1);
    };

} // namespace nitro::renderer
