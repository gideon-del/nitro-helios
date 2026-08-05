#include "nitro-renderer/passes/particle-indirect-pass.h"

namespace nitro::renderer
{
    ParticleIndirectPass::ParticleIndirectPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal) : m_device(device)
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
        computePipelineDesc.hasPushConstant = false;

        computePipelineDesc.threadGroupSizeX = 1;
        computePipelineDesc.threadGroupSizeY = 1;
        computePipelineDesc.threadGroupSizeZ = 1;
        computePipelineDesc.layouts = {m_descriptorLayout};

        std::string shaderPath = shaderDir + "/particle-indirect-copy/particle-indirect-copy";

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
    ParticleIndirectPass::~ParticleIndirectPass()
    {
        m_device->destroyDescriptorSet(m_descriptorSet);
        m_device->destroyComputePipeline(m_computePipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }
    void ParticleIndirectPass::bindResources(const RGResources &resources, const ParticleIndirectResourceIDs ids)
    {
        m_descriptorSet->writeBuffer(resources.getBuffer(ids.aliveCount), 2);
        m_descriptorSet->writeBuffer(resources.getBuffer(ids.indirectDraw), 3);
        m_descriptorSet->writeBuffer(resources.getBuffer(ids.indirectDispatch), 4);
        m_descriptorSet->commit();
    }

    void ParticleIndirectPass::execute(rhi::RHICommandBuffer *cmd)
    {
        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(m_descriptorSet, 0);
        cmd->dispatch(1, 1, 1);
    }
} // namespace nitro::renderer
