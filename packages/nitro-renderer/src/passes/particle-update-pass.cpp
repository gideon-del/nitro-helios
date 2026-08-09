#include <nitro-renderer/passes/particle-update-pass.h>
#include <random>
namespace nitro::renderer
{
    ParticleUpdatePass::ParticleUpdatePass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal)
        : m_device(device)
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
        computePipelineDesc.pushConstantSize = sizeof(ParticlePushConstant);
        computePipelineDesc.threadGroupSizeX = 64;
        computePipelineDesc.threadGroupSizeY = 1;
        computePipelineDesc.threadGroupSizeZ = 1;
        computePipelineDesc.layouts = {m_descriptorLayout};

        std::string shaderPath = shaderDir + "/particle-update/particle-update";

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

    ParticleUpdatePass::~ParticleUpdatePass()
    {
        m_device->destroyDescriptorSet(m_descriptorSet);
        m_device->destroyComputePipeline(m_computePipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void ParticleUpdatePass::uploadDeadList(const RGResources &resources, RGBufferID deadListID)
    {
        if (m_uploadedParticles)
            return;
        rhi::RHIBuffer *deadListBuffer = resources.getBuffer(deadListID);
        uint initialCount = s_MAX_PARTICLE_COUNT;
        std::vector<uint> initialParticles(s_MAX_PARTICLE_COUNT);

        for (uint i = 0; i < s_MAX_PARTICLE_COUNT; i++)
        {
            initialParticles[i] = i;
        }
        deadListBuffer->upload(&initialCount, sizeof(uint), 0);
        deadListBuffer->upload(initialParticles.data(), sizeof(uint) * s_MAX_PARTICLE_COUNT, sizeof(uint));
        m_uploadedParticles = true;
    }
    void ParticleUpdatePass::bindResources(const RGResources &resources, const ParticleUpdateResourceIDs ids)
    {
        m_descriptorSet->writeBuffer(resources.getBuffer(ids.particleId), 2);
        m_descriptorSet->writeBuffer(resources.getBuffer(ids.aliveIndicesId), 3);
        m_descriptorSet->writeBuffer(resources.getBuffer(ids.aliveCounterId), 4);
        m_descriptorSet->writeBuffer(resources.getBuffer(ids.deadListId), 5);
        m_descriptorSet->writeBuffer(resources.getBuffer(ids.emitterId), 6);
        m_descriptorSet->commit();
    }
    void ParticleUpdatePass::execute(rhi::RHICommandBuffer *cmd, ParticlePushConstant pc, rhi::RHIBuffer *indirectDispatch)
    {
        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(m_descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(ParticlePushConstant), 1, true);
        cmd->dispatchIndirect(indirectDispatch);
    }
} // namespace nitro::renderer
