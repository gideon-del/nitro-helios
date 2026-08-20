#include "nitro-renderer/passes/copy-hiz-depth.h"

namespace nitro::renderer
{
    CopyHizDepthPass::CopyHizDepthPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal) : m_device(device)
    {

        std::vector<rhi::RHIDescriptorBinding> bindings{
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             2},
            {rhi::RHIDescriptorBinding::Type::StorageImage,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             3},
        };

        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(CopyHizDepthPushConstant);

        computePipelineDesc.threadGroupSizeX = 16;
        computePipelineDesc.threadGroupSizeY = 16;
        computePipelineDesc.threadGroupSizeZ = 1;
        computePipelineDesc.layouts = {m_descriptorLayout};

        std::string shaderPath = shaderDir + "/copy-hiz-depth/copy-hiz-depth";

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
                CopyHizDepthResource resource;

                resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

                return resource;
            });
    }

    CopyHizDepthPass::~CopyHizDepthPass()
    {
        for (auto &resource : m_resources)
        {
            m_device->destroyDescriptorSet(resource.descriptorSet);
        }
        m_device->destroyComputePipeline(m_computePipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    bool CopyHizDepthPass::isStaleDescriptorSet(CopyHizDepthResource &resource, const CopyHizDepthRGResource &rgResources)
    {

        return resource.lastDepthTexture != rgResources.depthTexture || resource.lastHizTexture != rgResources.hizTexture;
    }

    void CopyHizDepthPass::bindDescriptorSet(CopyHizDepthResource &resource, const CopyHizDepthRGResource &rgResources)
    {
        resource.lastDepthTexture = rgResources.depthTexture;
        resource.lastHizTexture = rgResources.hizTexture;

        rhi::TextureBinding binding;
        binding.texture = resource.lastDepthTexture;
        binding.sampler = m_device->defaultSamplers().linearRepeat;

        resource.descriptorSet->writeTexture(binding, 2, rhi::ImageLayout::ShaderReadOnly);

        rhi::TextureSubresource subresource{};

        resource.descriptorSet->writeStorageImage(resource.lastHizTexture, 3, rhi::ImageLayout::General, subresource);

        resource.descriptorSet->commit();
    }

    void CopyHizDepthPass::execute(rhi::RHICommandBuffer *cmd, CopyHizDepthPushConstant &pc, const CopyHizDepthRGResource &rgResources)
    {
        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());

        if (isStaleDescriptorSet(resource, rgResources))
        {
            bindDescriptorSet(resource, rgResources);
        }

        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(resource.descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(CopyHizDepthPushConstant), 1, true);

        uint32_t groupSizeX = (pc.textureSize.x + 15) / 16;
        uint32_t groupSizeY = (pc.textureSize.y + 15) / 16;

        cmd->dispatch(groupSizeX, groupSizeY, 1);
    };

} // namespace nitro::renderer
