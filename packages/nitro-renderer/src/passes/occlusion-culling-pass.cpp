#include "nitro-renderer/passes/occlusion-culling-pass.h"

namespace nitro::renderer
{
    OcclusionCullingPass::OcclusionCullingPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal) : m_device(device)
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
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             7},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             8},
        };

        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(OcclusionCullPushConstant);

        computePipelineDesc.threadGroupSizeX = 64;
        computePipelineDesc.threadGroupSizeY = 1;
        computePipelineDesc.threadGroupSizeZ = 1;
        computePipelineDesc.layouts = {m_descriptorLayout};

        std::string shaderPath = shaderDir + "/occlusion-culling/occlusion-culling";

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
                OcclusionCullResource resource;

                resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
                return resource;
            });
    }

    OcclusionCullingPass::~OcclusionCullingPass()
    {
        m_device->destroyComputePipeline(m_computePipeline);
        for (auto &resource : m_resources)
        {
            m_device->destroyDescriptorSet(resource.descriptorSet);
        }
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }
    bool OcclusionCullingPass::isSceneStale(Scene &scene, OcclusionCullResource &resource, const OcclusionCullRGResource &rgResources)
    {

        return resource.lastMeshDescriptorBuffer != scene.meshManager->descriptorBuffer() || resource.lastMeshInstanceBuffer != scene.meshManager->instanceBuffer() || resource.lastSceneDrawCommandBuffer != rgResources.sceneDrawCommands || resource.lastSceneDrawCountBuffer != rgResources.sceneDrawCount || resource.lastHiZDrawCommandBuffer != rgResources.hiZDrawCommands || resource.lastHiZDrawCountBuffer != rgResources.hiZDrawCount || resource.lastHizDepthTexture != rgResources.hizDepthTex;
    };

    void OcclusionCullingPass::bindSceneResources(Scene &scene, OcclusionCullResource &resource, const OcclusionCullRGResource &rgResources)
    {
        resource.lastMeshDescriptorBuffer = scene.meshManager->descriptorBuffer();
        resource.lastMeshInstanceBuffer = scene.meshManager->instanceBuffer();
        resource.lastSceneDrawCommandBuffer = rgResources.sceneDrawCommands;
        resource.lastSceneDrawCountBuffer = rgResources.sceneDrawCount;
        resource.lastHiZDrawCommandBuffer = rgResources.hiZDrawCommands;
        resource.lastHiZDrawCountBuffer = rgResources.hiZDrawCount;
        resource.lastHizDepthTexture = rgResources.hizDepthTex;

        resource.descriptorSet->writeBuffer(resource.lastMeshDescriptorBuffer, 2);
        resource.descriptorSet->writeBuffer(resource.lastMeshInstanceBuffer, 3);
        resource.descriptorSet->writeBuffer(resource.lastSceneDrawCountBuffer, 4);
        resource.descriptorSet->writeBuffer(resource.lastSceneDrawCommandBuffer, 5);
        resource.descriptorSet->writeBuffer(resource.lastHiZDrawCountBuffer, 6);
        resource.descriptorSet->writeBuffer(resource.lastHiZDrawCommandBuffer, 7);
        rhi::TextureBinding binding;
        binding.texture = resource.lastHizDepthTexture;
        binding.sampler = m_device->defaultSamplers().linearClamp;
        resource.descriptorSet->writeTexture(binding, 8, rhi::ImageLayout::ShaderReadOnly);
        resource.descriptorSet->commit();
    }

    void OcclusionCullingPass::execute(rhi::RHICommandBuffer *cmd, OcclusionCullPushConstant &pc, Scene &scene, const OcclusionCullRGResource &rgResources)
    {

        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());

        if (isSceneStale(scene, resource, rgResources))
        {
            bindSceneResources(scene, resource, rgResources);
        }

        cmd->fillBuffer(rgResources.hiZDrawCount, 0, sizeof(uint32_t), 0u);

        rhi::BufferBarrier bufferBarrier;
        bufferBarrier.buffer = rgResources.hiZDrawCount;
        bufferBarrier.before = rhi::ResourceState::CopyDst;
        bufferBarrier.after = rhi::ResourceState::ShaderWrite;

        cmd->bufferBarrier(bufferBarrier);

        auto instanceCount = static_cast<uint32_t>(scene.instanceIds.size());
        uint32_t groupSizeX = (instanceCount + 63) / 64;
        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(resource.descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(OcclusionCullPushConstant), 1, true);
        cmd->dispatch(groupSizeX, 1, 1);
    }
} // namespace nitro::renderer
