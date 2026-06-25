#include <nitro-renderer/passes/tiled-deffered-compute-pass.h>

namespace nitro::renderer
{
    TiledLightingComputePass::TiledLightingComputePass(
        std::shared_ptr<rhi::RHIDevice> device,
        uint32_t width,
        uint32_t height,
        uint32_t maxPointLights,
        GBuffer &gBuffer,
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
            computePipelineDesc.shader.filePath = shaderPath + ".metal.lib";
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
                           [&, gBuffer](uint32_t frameIdx)
                           {
                               TileLightingComputeResource resource;
                               m_createBuffers(resource);

                               resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
                               resource.descriptorSet->writeBuffer(resource.cameraUniformBuffer, 2);
                               resource.descriptorSet->writeTexture(gBuffer.depth, 3, rhi::ImageLayout::ShaderReadOnly);
                               resource.descriptorSet->writeBuffer(resource.pointLightBuffer, 4);
                               resource.descriptorSet->writeBuffer(resource.tileLightCountBuffer, 5);
                               resource.descriptorSet->writeBuffer(resource.tileLightIndicesBuffer, 6);

                               resource.descriptorSet->commit();

                               return resource;
                           });
    }

    void TiledLightingComputePass::m_destroyBuffers()
    {
        for (auto &resource : m_resources)
        {
            m_device->destroyBuffer(resource.pointLightBuffer);
            m_device->destroyBuffer(resource.tileLightCountBuffer);
            m_device->destroyBuffer(resource.tileLightIndicesBuffer);
            m_device->destroyBuffer(resource.cameraUniformBuffer);
        }
    }

    TiledLightingComputePass::~TiledLightingComputePass()
    {

        m_destroyBuffers();
        for (auto &resource : m_resources)
        {

            m_device->destroyDescriptorSet(resource.descriptorSet);
        }

        m_device->destroyComputePipeline(m_computePipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    };

    void TiledLightingComputePass::m_createBuffers(TileLightingComputeResource &resource)
    {

        rhi::BufferDesc uboDesc;
        uint32_t totalTiles = m_tileSizeX * m_tileSizeY;
        uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
        uboDesc.usage = rhi::BufferDesc::Usage::Uniform;
        uboDesc.size = sizeof(TiledCameraUBO);

        resource.cameraUniformBuffer = m_device->createBuffer(uboDesc);

        rhi::BufferDesc lightDesc;

        lightDesc.storage = rhi::BufferDesc::StorageMode::Shared;
        lightDesc.usage = rhi::BufferDesc::Usage::Storage;
        lightDesc.size = sizeof(PointLight) * m_maxPointLights;

        resource.pointLightBuffer = m_device->createBuffer(lightDesc);

        rhi::BufferDesc lightCountDesc;

        lightCountDesc.storage = rhi::BufferDesc::StorageMode::Shared;
        lightCountDesc.usage = rhi::BufferDesc::Usage::Storage;
        lightCountDesc.size = sizeof(uint) * totalTiles;

        resource.tileLightCountBuffer = m_device->createBuffer(lightCountDesc);

        rhi::BufferDesc lightIndicesDesc;

        lightIndicesDesc.storage = rhi::BufferDesc::StorageMode::Shared;
        lightIndicesDesc.usage = rhi::BufferDesc::Usage::Storage;
        lightIndicesDesc.size = sizeof(uint) * totalTiles * TiledLightingComputePass::c_MAX_LIGHT_PER_TILE;

        resource.tileLightIndicesBuffer = m_device->createBuffer(lightIndicesDesc);
    };

    void TiledLightingComputePass::resize(uint32_t width, uint32_t height, GBuffer &gBuffer)
    {
        m_width = width;
        m_height = height;
        m_tileSizeX = static_cast<uint32_t>(ceil(float(m_width) / float(TiledLightingComputePass::c_TILE_GROUP_SIZE)));
        m_tileSizeY = static_cast<uint32_t>(ceil(float(m_height) / float(TiledLightingComputePass::c_TILE_GROUP_SIZE)));

        m_destroyBuffers();
        for (auto &resource : m_resources)
        {

            m_createBuffers(resource);

            resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
            resource.descriptorSet->writeBuffer(resource.cameraUniformBuffer, 2);
            resource.descriptorSet->writeTexture(gBuffer.depth, 3, rhi::ImageLayout::ShaderReadOnly);
            resource.descriptorSet->writeBuffer(resource.pointLightBuffer, 4);
            resource.descriptorSet->writeBuffer(resource.tileLightCountBuffer, 5);
            resource.descriptorSet->writeBuffer(resource.tileLightIndicesBuffer, 6);

            resource.descriptorSet->commit();
        }
    }

    void TiledLightingComputePass::execute(rhi::RHICommandBuffer *cmd, LightingSettings &settings, TiledCameraUBO cameraUBO)
    {

        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());

        resource.cameraUniformBuffer->upload(&cameraUBO, sizeof(TiledCameraUBO));
        resource.pointLightBuffer->upload(settings.pointLights.data(), sizeof(PointLight) * m_maxPointLights);
        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(resource.descriptorSet, 0);
        cmd->dispatch(m_tileSizeX, m_tileSizeY, 1);
        cmd->bufferBarrier(resource.tileLightCountBuffer);
        cmd->bufferBarrier(resource.tileLightIndicesBuffer);
    };

} // namespace nitro::renderer
