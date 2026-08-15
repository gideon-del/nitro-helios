#include "nitro-renderer/passes/hiz-mip-pass.h"

namespace nitro::renderer
{
    HiZMipPass::HiZMipPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal) : m_device(device)
    {
        std::vector<rhi::RHIDescriptorBinding> bindings{
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             2},
            {rhi::RHIDescriptorBinding::Type::StorageImage,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             3}};

        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(HizMipPushConstant);

        computePipelineDesc.threadGroupSizeX = 8;
        computePipelineDesc.threadGroupSizeY = 8;
        computePipelineDesc.threadGroupSizeZ = 1;
        computePipelineDesc.layouts = {m_descriptorLayout};

        std::string shaderPath = shaderDir + "/hiz-mip-gen/hiz-mip-gen";

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
                HizMipResource resource;

                for (int i = 0; i < HIZ_MIP_COUNT; i++)
                {
                    resource.descriptorSets[i] = m_device->createDescriptorSet(m_descriptorLayout);
                }

                return resource;
            });
    }

    HiZMipPass::~HiZMipPass()
    {
        m_device->destroyComputePipeline(m_computePipeline);

        for (auto &resource : m_resources)
        {

            for (int i = 0; i < HIZ_MIP_COUNT; i++)
            {
                m_device->destroyDescriptorSet(resource.descriptorSets[i]);
            }
        }

        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void HiZMipPass::bindResources(const RGResources &resources, const RGTextureID &hizTexID)
    {
        rhi::RHITexture *hizTexture = resources.getTexture(hizTexID);

        for (auto &resource : m_resources)
        {
            for (int i = 0; i < HIZ_MIP_COUNT; i++)
            {
                rhi::TextureBinding binding;
                binding.texture = hizTexture;
                binding.sampler = m_device->defaultSamplers().linearRepeat;

                rhi::TextureSubresource subResource{};
                subResource.baseMip = i;

                resource.descriptorSets[i]->writeTextureMip(binding, 2, rhi::ImageLayout::ShaderReadOnly, subResource);
                subResource.baseMip = i + 1;
                resource.descriptorSets[i]->writeStorageImage(hizTexture, 3, rhi::ImageLayout::General, subResource);
                resource.descriptorSets[i]->commit();
            }
        }
    }

    void HiZMipPass::execute(rhi::RHICommandBuffer *cmd, const uint32_t width, const uint32_t height, rhi::RHITexture *hizTexture)
    {
        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());

        for (int i = 0; i < HIZ_MIP_COUNT; i++)
        {

            uint32_t mipWidth = std::max(1u, width >> (i + 1));
            uint32_t mipHeight = std::max(1u, height >> (i + 1));

            uint32_t groupSizeX = (mipWidth + 7) / 8;
            uint32_t groupSizeY = (mipHeight + 7) / 8;

            HizMipPushConstant pc;
            pc.textureSize = {mipWidth, mipHeight};

            rhi::TextureBarrier barrier;
            barrier.texture = hizTexture;
            barrier.before = rhi::ResourceState::ShaderRead;
            barrier.after = rhi::ResourceState::ShaderWrite;
            barrier.subresource.baseMip = i + 1;

            cmd->textureBarrier(barrier);

            cmd->bindComputePipeline(m_computePipeline);
            cmd->bindComputeDescriptorSet(resource.descriptorSets[i], 0);
            cmd->setPushConstant(&pc, sizeof(HizMipPushConstant), 1, true);
            cmd->dispatch(groupSizeX, groupSizeY, 1);

            barrier.before = rhi::ResourceState::ShaderWrite;
            barrier.after = rhi::ResourceState::ShaderRead;

            cmd->textureBarrier(barrier);
        }
    };

} // namespace nitro::renderer
