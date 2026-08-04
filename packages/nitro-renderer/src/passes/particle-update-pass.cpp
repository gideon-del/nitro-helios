#include <nitro-renderer/passes/particle-update-pass.h>
#include <random>
namespace nitro::renderer
{
    ParticleUpdatePass::ParticleUpdatePass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal)
        : m_device(device)
    {
        std::vector<rhi::RHIDescriptorBinding> bindings{{rhi::RHIDescriptorBinding::Type::StorageBuffer,
                                                         rhi::RHIDescriptorBinding::ShaderStage::Compute,
                                                         2}};

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

    void ParticleUpdatePass::uploadInitalParticles(const RGResources &resources, RGBufferID particleId)
    {
        if (m_uploadedParticles)
            return;
        rhi::RHIBuffer *particleBuffer = resources.getBuffer(particleId);
        std::vector<ParticleDesc> initialParticles(s_MAX_PARTICLE_COUNT);
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> velocityDist(-1.0f, 1.0f);
        std::uniform_real_distribution<float> ageDist(0.0f, 5.0f);

        for (auto &p : initialParticles)
        {
            p.position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            p.velocity = glm::vec4(velocityDist(rng), velocityDist(rng), velocityDist(rng), 1.0f);
            p.age = ageDist(rng);
            p.color = glm::vec4(1.0f);
            p.lifetime = 5.0f;
            p.size = 0.05f;
        }

        particleBuffer->upload(initialParticles.data(), sizeof(ParticleDesc) * s_MAX_PARTICLE_COUNT);

        m_descriptorSet->writeBuffer(particleBuffer, 2);
        m_descriptorSet->commit();

        m_uploadedParticles = true;
    }

    void ParticleUpdatePass::execute(rhi::RHICommandBuffer *cmd, ParticlePushConstant pc, const RGResources &resources, const RGBufferID particleId)
    {
        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(m_descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(ParticlePushConstant), 1, true);

        uint32_t groupCount = (s_MAX_PARTICLE_COUNT + 63) / 64;
        cmd->dispatch(groupCount, 1, 1);
    }
} // namespace nitro::renderer
