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

        m_descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
    }

    MeshCompactPass::~MeshCompactPass()
    {
        m_device->destroyComputePipeline(m_computePipeline);
        m_device->destroyDescriptorSet(m_descriptorSet);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void MeshCompactPass::bindSceneBuffer(Scene &scene, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer)
    {

        m_lastMeshDescriptorBuffer = scene.meshManager->descriptorBuffer();
        m_lastMeshInstanceBuffer = scene.meshManager->instanceBuffer();
        m_lastSceneInstanceIdBuffer = scene.sceneInstanceIdBuffer();
        m_descriptorSet->writeBuffer(m_lastMeshDescriptorBuffer, 2);
        m_descriptorSet->writeBuffer(m_lastMeshInstanceBuffer, 3);
        m_descriptorSet->writeBuffer(drawCountBuffer, 4);
        m_descriptorSet->writeBuffer(drawCommandBuffer, 5);
        m_descriptorSet->writeBuffer(m_lastSceneInstanceIdBuffer, 6);
        m_descriptorSet->commit();
    }
    bool MeshCompactPass::isSceneBufferStale(Scene &scene)
    {
        return m_lastMeshDescriptorBuffer != scene.meshManager->descriptorBuffer() || m_lastMeshInstanceBuffer != scene.meshManager->instanceBuffer() || m_lastSceneInstanceIdBuffer != scene.sceneInstanceIdBuffer();
    }

    void MeshCompactPass::execute(rhi::RHICommandBuffer *cmd, Scene &scene, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer, MeshCompactPushConstant &pc)
    {

        cmd->fillBuffer(drawCountBuffer, 0, sizeof(uint32_t), 0u);

        rhi::BufferBarrier barrier;
        barrier.buffer = drawCountBuffer;
        barrier.before = rhi::ResourceState::CopyDst;
        barrier.after = rhi::ResourceState::ShaderWrite;
        cmd->bufferBarrier(barrier);

        if (isSceneBufferStale(scene))
        {
            bindSceneBuffer(scene, drawCommandBuffer, drawCountBuffer);
        }

        uint32_t groupSize = (pc.objectCount + 63) / 64;
        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(m_descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(MeshCompactPushConstant), 1, true);
        cmd->dispatch(groupSize, 1, 1);
    }

} // namespace nitro::renderer
