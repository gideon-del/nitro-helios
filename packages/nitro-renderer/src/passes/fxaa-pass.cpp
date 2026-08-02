#include <nitro-renderer/passes/fxaa-pass.h>

namespace nitro::renderer
{
    FXAAPass::FXAAPass(std::shared_ptr<rhi::RHIDevice> device,
                       uint32_t width,
                       uint32_t height,

                       std::string shaderDir,
                       bool isMetal)
        : m_device(device),
          m_width(width),
          m_height(height)
    {
        std::vector<rhi::RHIDescriptorBinding> binding{
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             2},
            {rhi::RHIDescriptorBinding::Type::StorageImage,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             3},
        };

        m_descriptorLayout = m_device->createDescriptorLayout(binding);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.layouts = {m_descriptorLayout};
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(FXAAPushConstant);
        std::string shaderPath = shaderDir + "/fxaa/fxaa";

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

        m_resources.create(g_MAX_FRAMES_IN_FLIGHT,
                           [&](uint32_t frameIdx)
                           {
                               SingleInputPassResource resource;
                               resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
                               return resource;
                           });
    }

    FXAAPass::~FXAAPass()
    {
        for (auto &resource : m_resources)
        {
            m_device->destroyDescriptorSet(resource.descriptorSet);
        }
        m_device->destroyComputePipeline(m_computePipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void FXAAPass::resize(uint32_t width, uint32_t height)
    {

        m_width = width;
        m_height = height;
    }

    void FXAAPass::execute(rhi::RHICommandBuffer *cmd, FXAAPushConstant pc, FXAATextures textures)
    {

        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());
        if (resource.lastInputTexture != textures.ldrTexture)
        {
            resource.lastInputTexture = textures.ldrTexture;

            rhi::TextureBinding textureBinding;
            textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
            textureBinding.texture = textures.ldrTexture;
            resource.descriptorSet->writeTexture(textureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
            resource.descriptorSet->writeStorageImage(textures.output, 3, rhi::ImageLayout::General, rhi::TextureSubresource{});
            resource.descriptorSet->commit();
        }

        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(resource.descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(FXAAPushConstant), 1, true);

        uint32_t groupSizeX = (m_width + 15) / 16;
        uint32_t groupSizeY = (m_height + 15) / 16;

        cmd->dispatch(groupSizeX, groupSizeY, 1);
    }
} // namespace nitro::renderer
