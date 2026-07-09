#include <nitro-renderer/utils.h>
#include <stb_image.h>
namespace nitro::renderer
{
    struct CubemapPushConstant
    {
        uint face;
        uint faceSize;
        float _pads[2];
    };
    rhi::RHITexture *loadHDRImage(std::shared_ptr<rhi::RHIDevice> device, std::string filePath)
    {
        int width, height, channels;
        auto *raw = stbi_loadf(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!raw)
        {
            throw std::runtime_error("File with path " + filePath + " not found");
        }

        rhi::TextureDesc textureDesc;

        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA32;
        textureDesc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        textureDesc.usage = rhi::TextureDesc::Usage::ShaderRead;
        textureDesc.initialData = raw;

        rhi::RHITexture *texture = device->createTexture(textureDesc);

        stbi_image_free(raw);

        return texture;
    }

    rhi::RHITexture *createCubeMap(std::shared_ptr<rhi::RHIDevice> device, rhi::RHITexture *sourceTexture, uint32_t size, std::string shaderDir, bool isMetal)
    {
        rhi::TextureDesc cubemapTextureDesc;
        cubemapTextureDesc.size = {size, size};
        cubemapTextureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
        cubemapTextureDesc.type = rhi::TextureDesc::Type::Cube;
        cubemapTextureDesc.usage = rhi::TextureDesc::Usage::Storage | rhi::TextureDesc::Usage::ShaderRead;

        rhi::RHITexture *cubemapTexture = device->createTexture(cubemapTextureDesc);

        std::vector<rhi::RHIDescriptorBinding> binding{
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Compute, 2},
            {rhi::RHIDescriptorBinding::Type::StorageImage, rhi::RHIDescriptorBinding::ShaderStage::Compute, 3}};
        rhi::RHIDescriptorLayout *descriptorLayout = device->createDescriptorLayout(binding);
        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.layouts = {descriptorLayout};
        computePipelineDesc.pushConstantSize = sizeof(CubemapPushConstant);
        rhi::ShaderDesc shaderDesc;
        shaderDesc.stage = rhi::ShaderStage::Compute;
        std::string shaderPath = shaderDir + "/cube-map/cube-map";

        if (isMetal)
        {
            shaderDesc.filePath = shaderPath + ".metallib";
            shaderDesc.name = "comp";
        }
        else
        {
            shaderDesc.filePath = shaderPath + ".comp.spv";
            shaderDesc.name = "main";
        }
        computePipelineDesc.shader = shaderDesc;

        rhi::RHIComputePipeline *computePipeline = device->createComputePipeline(computePipelineDesc);
        std::vector<rhi::RHIDescriptorSet *> descriptorSets;
        for (int i = 0; i < 6; i++)
        {

            rhi::RHIDescriptorSet *descriptorSet = device->createDescriptorSet(descriptorLayout);
            descriptorSet->writeTexture(sourceTexture, 2, rhi::ImageLayout::ShaderReadOnly);
            descriptorSet->writeStorageImage(cubemapTexture, 3, rhi::ImageLayout::General, i);
            descriptorSet->commit();
            descriptorSets.push_back(descriptorSet);
        }
        rhi::RHICommandBuffer *cmd = device->createCommandBuffer();
        std::cout << "Done with Descriptor set" << std::endl;
        for (int face = 0; face < 6; face++)
        {
            std::cout << "First face " << face << std::endl;
            rhi::RHIDescriptorSet *descriptorSet = descriptorSets[face];

            std::cout << "Got descriptor set  " << std::endl;
            cmd->bindComputePipeline(computePipeline);
            cmd->bindComputeDescriptorSet(descriptorSet, 0);
            CubemapPushConstant pc;
            pc.face = face;
            pc.faceSize = size;
            cmd->setPushConstant(&pc, sizeof(CubemapPushConstant), 1, true);
            uint32_t groups = (size + 15) / 16;
            cmd->dispatch(groups, groups, 1);
        }

        device->endCommandBuffer(cmd);
        device->destroyComputePipeline(computePipeline);

        for (auto descriptorSet : descriptorSets)
        {
            device->destroyDescriptorSet(descriptorSet);
        }
        device->destroyDescriptorLayout(descriptorLayout);

        return cubemapTexture;
    }
} // namespace nitro::renderer
