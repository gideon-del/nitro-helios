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

        m_resources.create(g_MAX_FRAMES_IN_FLIGHT,
                           [&](uint32_t frameIdx)
                           {
                               CombineTexturePassResource resource;
                               resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
                               return resource;
                           });
    }

    CombineTexturePass::~CombineTexturePass()
    {
        m_device->destroyComputePipeline(m_computePipeline);

        for (auto &resource : m_resources)
        {
            m_device->destroyDescriptorSet(resource.descriptorSet);
        }
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void CombineTexturePass::resize(uint32_t width, uint32_t height)
    {

        m_width = width;
        m_height = height;
    }

    void CombineTexturePass::execute(rhi::RHICommandBuffer *cmd, CombineTexturePushConstant pc, CombineTexturePassTextures textures)
    {
        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());

        if (resource.lastHdrTexture != textures.hdrTexture || resource.lastBlurredTexture != textures.blurredTexture)
        {
            resource.lastHdrTexture = textures.hdrTexture;
            resource.lastBlurredTexture = textures.blurredTexture;

            rhi::TextureBinding textureBinding;
            textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
            textureBinding.texture = textures.hdrTexture;
            resource.descriptorSet->writeTexture(textureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
            textureBinding.texture = textures.blurredTexture;
            resource.descriptorSet->writeTexture(textureBinding, 3, rhi::ImageLayout::ShaderReadOnly);
            resource.descriptorSet->writeStorageImage(textures.output, 4, rhi::ImageLayout::General, rhi::TextureSubresource{});
            resource.descriptorSet->commit();
        }

        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(resource.descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(CombineTexturePushConstant), 1, true);
        uint32_t groupSizeX = (m_width + 15) / 16;
        uint32_t groupSizeY = (m_height + 15) / 16;

        cmd->dispatch(groupSizeX, groupSizeY, 1);
    };

} // namespace nitro::renderer
