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

        m_descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
    }

    FXAAPass::~FXAAPass()
    {
        m_device->destroyDescriptorSet(m_descriptorSet);
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

        // if (m_lastLDRTexture != textures.ldrTexture)
        // {

        // }
        m_lastLDRTexture = textures.ldrTexture;

        rhi::TextureBinding textureBinding;
        textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
        textureBinding.texture = textures.ldrTexture;
        m_descriptorSet->writeTexture(textureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
        m_descriptorSet->writeStorageImage(textures.output, 3, rhi::ImageLayout::General, rhi::TextureSubresource{});
        m_descriptorSet->commit();

        rhi::TextureBarrier textureBarrier;
        textureBarrier.texture = textures.output;
        textureBarrier.before = rhi::ResourceState::ShaderRead;
        textureBarrier.after = rhi::ResourceState::ShaderWrite;
        cmd->textureBarrier(textureBarrier);
        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(m_descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(FXAAPushConstant), 1, true);

        uint32_t groupSizeX = (m_width + 15) / 16;
        uint32_t groupSizeY = (m_height + 15) / 16;

        cmd->dispatch(groupSizeX, groupSizeY, 1);

        textureBarrier.before = rhi::ResourceState::ShaderWrite;
        textureBarrier.after = rhi::ResourceState::ShaderRead;

        cmd->textureBarrier(textureBarrier);
    }
} // namespace nitro::renderer
