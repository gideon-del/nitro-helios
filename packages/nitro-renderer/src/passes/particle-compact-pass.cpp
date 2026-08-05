#include <nitro-renderer/passes/particle-compact-pass.h>

namespace nitro::renderer
{
    ParticleCompactPass::ParticleCompactPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal) : m_device(device)
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
        };

        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(ParticleCompactPushConstant);
        computePipelineDesc.threadGroupSizeX = 64;
        computePipelineDesc.threadGroupSizeY = 1;
        computePipelineDesc.threadGroupSizeZ = 1;
        computePipelineDesc.layouts = {m_descriptorLayout};

        std::string shaderPath = shaderDir + "/particle-compact/particle-compact";

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
    };

    void ParticleCompactPass::bindResources(const RGResources &resources, const ParticleCompactResourceIDs &ids)
    {
        m_descriptorSet->writeBuffer(resources.getBuffer(ids.particleIds), 2);
        m_descriptorSet->writeBuffer(resources.getBuffer(ids.aliveIndicesId), 3);
        m_descriptorSet->writeBuffer(resources.getBuffer(ids.aliveCountId), 4);
        m_descriptorSet->commit();
    }

    void ParticleCompactPass::execute(rhi::RHICommandBuffer *cmd, ParticleCompactPushConstant &pc)
    {
        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(m_descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(ParticleCompactPushConstant), 1, true);

        uint32_t groupX = (pc.particleCount + 63) / 64;

        cmd->dispatch(groupX, 1, 1);
    }

    ParticleCompactPass::~ParticleCompactPass()
    {
        m_device->destroyDescriptorSet(m_descriptorSet);
        m_device->destroyComputePipeline(m_computePipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }
} // namespace nitro::renderer
