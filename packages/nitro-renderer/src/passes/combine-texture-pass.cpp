#include <nitro-renderer/passes/combine-texture-pass.h>

namespace nitro::renderer
{

    CombineTexturePass::CombineTexturePass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal)
        : m_device(device),
          m_width(width),
          m_height(height)
    {
        std::vector<rhi::RHIDescriptorBinding> binding{
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             2},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             3},
            {rhi::RHIDescriptorBinding::Type::StorageImage,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             4},

        };

        m_descriptorLayout = m_device->createDescriptorLayout(binding);
        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(CombineTexturePushConstant);
        computePipelineDesc.layouts = {m_descriptorLayout};
        computePipelineDesc.shader.stage = rhi::ShaderStage::Compute;
        std::string shaderPath = shaderDir + "/combine-texture/combine-texture";
        if (isMetal)
        {
            computePipelineDesc.shader.filePath = shaderPath + ".metallib";
            computePipelineDesc.shader.name = "comp";
        }
        else
        {
            computePipelineDesc.shader.filePath = shaderPath + ".comp.spv";
            computePipelineDesc.shader.name = "main";
        }

        m_computePipeline = m_device->createComputePipeline(computePipelineDesc);

        m_descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
    };

    CombineTexturePass::~CombineTexturePass()
    {
        m_device->destroyComputePipeline(m_computePipeline);

        m_device->destroyDescriptorSet(m_descriptorSet);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void CombineTexturePass::resize(uint32_t width, uint32_t height)
    {

        m_width = width;
        m_height = height;
    }

    void CombineTexturePass::execute(rhi::RHICommandBuffer *cmd, CombineTexturePushConstant pc, CombineTexturePassTextures textures)
    {
        rhi::TextureBarrier initialTextureBarrier;
        initialTextureBarrier.texture = textures.output;
        initialTextureBarrier.before = rhi::ResourceState::ShaderRead;
        initialTextureBarrier.after = rhi::ResourceState::ShaderWrite;
        cmd->textureBarrier(initialTextureBarrier);

        rhi::TextureBinding textureBinding;
        textureBinding.texture = textures.hdrTexture;
        textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
        m_descriptorSet->writeTexture(textureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
        textureBinding.texture = textures.blurredTexture;
        m_descriptorSet->writeTexture(textureBinding, 3, rhi::ImageLayout::ShaderReadOnly);
        m_descriptorSet->writeStorageImage(textures.output, 4, rhi::ImageLayout::General, rhi::TextureSubresource{});
        m_descriptorSet->commit();

        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(m_descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(CombineTexturePushConstant), 1, true);
        uint32_t groupSizeX = (m_width + 15) / 16;
        uint32_t groupSizeY = (m_height + 15) / 16;

        cmd->dispatch(groupSizeX, groupSizeY, 1);
        rhi::TextureBarrier finalTextureBarrier;
        finalTextureBarrier.texture = textures.output;
        finalTextureBarrier.before = rhi::ResourceState::ShaderWrite;
        finalTextureBarrier.after = rhi::ResourceState::ShaderRead;
        cmd->textureBarrier(finalTextureBarrier);
    };

} // namespace nitro::renderer
