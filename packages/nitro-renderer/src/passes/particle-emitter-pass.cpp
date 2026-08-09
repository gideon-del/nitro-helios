#include "nitro-renderer/passes/particle-emitter-pass.h"

namespace nitro::renderer
{
    ParticleEmitterPass::ParticleEmitterPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal)
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
        };

        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(ParticleEmitterPushConstant);
        computePipelineDesc.threadGroupSizeX = 64;
        computePipelineDesc.threadGroupSizeY = 1;
        computePipelineDesc.threadGroupSizeZ = 1;
        computePipelineDesc.layouts = {m_descriptorLayout};

        std::string shaderPath = shaderDir + "/particle-spawn/particle-spawn";

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

    ParticleEmitterPass::~ParticleEmitterPass()
    {
        m_device->destroyDescriptorSet(m_descriptorSet);
        m_device->destroyComputePipeline(m_computePipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void ParticleEmitterPass::uploadInitialEmitter(const RGResources &resources, const RGBufferID emitterID)
    {
        // if (m_hasUploadedEmitters)
        //     return;

        // EmitterDesc fireEmitter;
        // fireEmitter.position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        // fireEmitter.direction = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        // fireEmitter.startColor = glm::vec4(1.0f, 0.9f, 0.3f, 1.0f);
        // fireEmitter.endColor = glm::vec4(0.8f, 0.15f, 0.0f, 1.0f);
        // fireEmitter.spawnRate = 1000.0f;
        // fireEmitter.initialSpeed = 1.5f;
        // fireEmitter.speedVariance = 0.5f;
        // fireEmitter.spread = glm::radians(15.0f);
        // fireEmitter.minLifetime = 0.8f;
        // fireEmitter.maxLifetime = 2.0f;
        // fireEmitter.startSize = 1.4f;
        // fireEmitter.endSize = 0.6f;

        // std::vector<EmitterDesc> initialEmitters(s_MAX_EMITTERS);
        // initialEmitters[0] = fireEmitter;

        // rhi::RHIBuffer *emitterBuffer = resources.getBuffer(emitterID);

        // emitterBuffer->upload(initialEmitters.data(), sizeof(EmitterDesc) * s_MAX_EMITTERS);
        // m_hasUploadedEmitters = true;
    }

    void ParticleEmitterPass::bindResources(const RGResources &resources, const ParticleEmitterBufferIDs &ids)
    {
        m_descriptorSet->writeBuffer(resources.getBuffer(ids.particleID), 2);
        m_descriptorSet->writeBuffer(resources.getBuffer(ids.deadListID), 3);
        m_descriptorSet->writeBuffer(resources.getBuffer(ids.emitterID), 4);
        m_descriptorSet->commit();
    }

    void ParticleEmitterPass::execute(rhi::RHICommandBuffer *cmd, ParticleEmitterPushConstant pc)
    {

        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(m_descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(ParticleEmitterPushConstant), 1, true);
        uint32_t groupX = (pc.emitterCount + 63) / 64;
        groupX = std::max(groupX, 1u);

        cmd->dispatch(groupX, 1, 1);
    };
} // namespace nitro::renderer
