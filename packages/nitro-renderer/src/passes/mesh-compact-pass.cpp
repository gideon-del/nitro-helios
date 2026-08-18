#include "nitro-renderer/passes/mesh-compace-pass.h"

namespace nitro::renderer
{
    MeshCompactPass::MeshCompactPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal) : m_device(device)
    {
        std::vector<rhi::RHIDescriptorBinding> bindings{
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             2},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
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
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             7},
        };

        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(MeshCompactPushConstant);

        computePipelineDesc.threadGroupSizeX = 64;
        computePipelineDesc.threadGroupSizeY = 1;
        computePipelineDesc.threadGroupSizeZ = 1;
        computePipelineDesc.layouts = {m_descriptorLayout};

        std::string shaderPath = shaderDir + "/mesh-compact/mesh-compact";

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

        m_resources.create(
            g_MAX_FRAMES_IN_FLIGHT,
            [&](uint32_t frameIdx)
            {
                MeshCompactResources resource;
                rhi::BufferDesc uboDesc;
                uboDesc.size = sizeof(MeshCompactUBO);
                uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                uboDesc.usage = rhi::BufferDesc::Usage::Uniform;

                resource.uniformBuffer = m_device->createBuffer(uboDesc);

                resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
                return resource;
            });
    }

    MeshCompactPass::~MeshCompactPass()
    {
        m_device->destroyComputePipeline(m_computePipeline);
        for (auto &resource : m_resources)
        {
            m_device->destroyDescriptorSet(resource.descriptorSet);
            m_device->destroyBuffer(resource.uniformBuffer);
        }
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void MeshCompactPass::bindSceneBuffer(Scene &scene, MeshCompactResources &resource, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer, rhi::RHITexture *hizTexture)
    {

        resource.lastMeshDescriptorBuffer = scene.meshManager->descriptorBuffer();
        resource.lastMeshInstanceBuffer = scene.meshManager->instanceBuffer();
        resource.lastSceneInstanceIdBuffer = scene.sceneInstanceIdBuffer();
        resource.lastDrawCommandBuffer = drawCommandBuffer;
        resource.lastDrawCountBuffer = drawCountBuffer;
        resource.lastHizTexture = hizTexture;

        resource.descriptorSet->writeBuffer(resource.lastMeshDescriptorBuffer, 2);
        resource.descriptorSet->writeBuffer(resource.lastMeshInstanceBuffer, 3);
        resource.descriptorSet->writeBuffer(resource.lastDrawCountBuffer, 4);
        resource.descriptorSet->writeBuffer(resource.lastDrawCommandBuffer, 5);
        resource.descriptorSet->writeBuffer(resource.lastSceneInstanceIdBuffer, 6);
        rhi::TextureBinding binding;
        binding.texture = resource.lastHizTexture;
        binding.sampler = m_device->defaultSamplers().linearClamp;
        resource.descriptorSet->writeTexture(binding, 7, rhi::ImageLayout::ShaderReadOnly);
        resource.descriptorSet->commit();
    }
    bool MeshCompactPass::isSceneBufferStale(Scene &scene, MeshCompactResources &resource, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer, rhi::RHITexture *hizTexture)
    {
        return resource.lastMeshDescriptorBuffer != scene.meshManager->descriptorBuffer() || resource.lastMeshInstanceBuffer != scene.meshManager->instanceBuffer() || resource.lastSceneInstanceIdBuffer != scene.sceneInstanceIdBuffer() || resource.lastDrawCommandBuffer != drawCommandBuffer || resource.lastDrawCountBuffer != drawCountBuffer || resource.lastHizTexture != hizTexture;

        ;
    }

    void MeshCompactPass::execute(rhi::RHICommandBuffer *cmd, Scene &scene, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer, MeshCompactPushConstant &pc, rhi::RHITexture *hizTexture)
    {

        cmd->fillBuffer(drawCountBuffer, 0, sizeof(uint32_t), 0u);

        rhi::BufferBarrier barrier;
        barrier.buffer = drawCountBuffer;
        barrier.before = rhi::ResourceState::CopyDst;
        barrier.after = rhi::ResourceState::ShaderWrite;
        cmd->bufferBarrier(barrier);

        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());
        if (isSceneBufferStale(scene, resource, drawCommandBuffer, drawCountBuffer, hizTexture))
        {
            bindSceneBuffer(scene, resource, drawCommandBuffer, drawCountBuffer, hizTexture);
        }

        uint32_t groupSize = (pc.objectCount + 63) / 64;
        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(resource.descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(MeshCompactPushConstant), 1, true);
        cmd->dispatch(groupSize, 1, 1);
    }

} // namespace nitro::renderer
