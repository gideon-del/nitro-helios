
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
    struct BRDFPushConstant
    {
        uint faceSize;
        float _pads[3];
    };
    struct PrefilterPushConstant
    {
        uint face;
        uint faceSize;
        float roughness;
        uint resolution;
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
        cubemapTextureDesc.mipmaps = 9;
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
            rhi::TextureSubresource subresource{};
            subresource.baseMip = 0;
            subresource.baseLayer = i;
            descriptorSet->writeStorageImage(cubemapTexture, 3, rhi::ImageLayout::General, subresource);
            descriptorSet->commit();
            descriptorSets.push_back(descriptorSet);
        }
        rhi::RHICommandBuffer *cmd = device->createCommandBuffer();
        rhi::TextureSubresource initialSubResource;
        initialSubResource.layerCount = 6;
        rhi::TextureBarrier textureBarrier{.texture = cubemapTexture,
                                           .before = rhi::ResourceState::Undefined,
                                           .after = rhi::ResourceState::ShaderWrite,
                                           .subresource = initialSubResource};

        cmd->textureBarrier(textureBarrier);

        for (int face = 0; face < 6; face++)
        {

            rhi::RHIDescriptorSet *descriptorSet = descriptorSets[face];

            cmd->bindComputePipeline(computePipeline);
            cmd->bindComputeDescriptorSet(descriptorSet, 0);
            CubemapPushConstant pc;
            pc.face = face;
            pc.faceSize = size;
            cmd->setPushConstant(&pc, sizeof(CubemapPushConstant), 1, true);
            uint32_t groups = (size + 15) / 16;
            cmd->dispatch(groups, groups, 1);
        }

        cmd->generateMipmaps(cubemapTexture);

        device->endCommandBuffer(cmd);
        device->destroyComputePipeline(computePipeline);

        for (auto descriptorSet : descriptorSets)
        {
            device->destroyDescriptorSet(descriptorSet);
        }
        device->destroyDescriptorLayout(descriptorLayout);

        return cubemapTexture;
    }

    rhi::RHITexture *generateIrradianceMap(
        std::shared_ptr<rhi::RHIDevice> device, rhi::RHITexture *environment, uint32_t size, std::string shaderDir, bool isMetal)
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
        std::string shaderPath = shaderDir + "/irradiance-map/irradiance-map";

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
            descriptorSet->writeTexture(environment, 2, rhi::ImageLayout::ShaderReadOnly);
            rhi::TextureSubresource subresource{};
            subresource.baseMip = 0;
            subresource.baseLayer = i;
            descriptorSet->writeStorageImage(cubemapTexture, 3, rhi::ImageLayout::General, subresource);
            descriptorSet->commit();
            descriptorSets.push_back(descriptorSet);
        }
        rhi::RHICommandBuffer *cmd = device->createCommandBuffer();
        rhi::TextureSubresource initialSubresource{};
        initialSubresource.layerCount = 6;
        rhi::TextureBarrier textureBarrier{.texture = cubemapTexture,
                                           .before = rhi::ResourceState::Undefined,
                                           .after = rhi::ResourceState::ShaderWrite,
                                           .subresource = initialSubresource};

        cmd->textureBarrier(textureBarrier);
        for (int face = 0; face < 6; face++)
        {

            rhi::RHIDescriptorSet *descriptorSet = descriptorSets[face];

            cmd->bindComputePipeline(computePipeline);
            cmd->bindComputeDescriptorSet(descriptorSet, 0);
            CubemapPushConstant pc;
            pc.face = face;
            pc.faceSize = size;
            cmd->setPushConstant(&pc, sizeof(CubemapPushConstant), 1, true);
            uint32_t groups = (size + 15) / 16;
            cmd->dispatch(groups, groups, 1);
        }

        rhi::TextureBarrier finalTextureBarrier{.texture = cubemapTexture,
                                                .before = rhi::ResourceState::ShaderWrite,
                                                .after = rhi::ResourceState::ShaderRead,
                                                .subresource = initialSubresource};

        cmd->textureBarrier(finalTextureBarrier);
        device->endCommandBuffer(cmd);
        device->destroyComputePipeline(computePipeline);

        for (auto descriptorSet : descriptorSets)
        {
            device->destroyDescriptorSet(descriptorSet);
        }
        device->destroyDescriptorLayout(descriptorLayout);

        return cubemapTexture;
    }
    rhi::RHITexture *generatePrefliteredMap(
        std::shared_ptr<rhi::RHIDevice> device, rhi::RHITexture *environment, uint32_t size, std::string shaderDir, bool isMetal)
    {
        rhi::TextureDesc cubemapTextureDesc;
        cubemapTextureDesc.size = {size, size};
        cubemapTextureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
        cubemapTextureDesc.type = rhi::TextureDesc::Type::Cube;
        cubemapTextureDesc.usage = rhi::TextureDesc::Usage::Storage | rhi::TextureDesc::Usage::ShaderRead;
        cubemapTextureDesc.mipmaps = 4;

        rhi::RHITexture *cubemapTexture = device->createTexture(cubemapTextureDesc);

        std::vector<rhi::RHIDescriptorBinding> binding{
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Compute, 2},
            {rhi::RHIDescriptorBinding::Type::StorageImage, rhi::RHIDescriptorBinding::ShaderStage::Compute, 3}};
        rhi::RHIDescriptorLayout *descriptorLayout = device->createDescriptorLayout(binding);
        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.layouts = {descriptorLayout};
        computePipelineDesc.pushConstantSize = sizeof(PrefilterPushConstant);
        rhi::ShaderDesc shaderDesc;
        shaderDesc.stage = rhi::ShaderStage::Compute;
        std::string shaderPath = shaderDir + "/prefiltered-map/prefiltered-map";

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
        for (int mip = 0; mip <= 4; mip++)
        {
            for (int i = 0; i < 6; i++)
            {

                rhi::RHIDescriptorSet *descriptorSet = device->createDescriptorSet(descriptorLayout);
                descriptorSet->writeTexture(environment, 2, rhi::ImageLayout::ShaderReadOnly);
                rhi::TextureSubresource subresource{};
                subresource.baseMip = mip;
                subresource.baseLayer = i;
                descriptorSet->writeStorageImage(cubemapTexture, 3, rhi::ImageLayout::General, subresource);
                descriptorSet->commit();
                descriptorSets.push_back(descriptorSet);
            }
        }
        rhi::RHICommandBuffer *cmd = device->createCommandBuffer();
        rhi::TextureSubresource initialSubresource{};
        initialSubresource.layerCount = 6;
        initialSubresource.mipCount = 5;
        rhi::TextureBarrier textureBarrier{.texture = cubemapTexture,
                                           .before = rhi::ResourceState::Undefined,
                                           .after = rhi::ResourceState::ShaderWrite,
                                           .subresource = initialSubresource};

        cmd->textureBarrier(textureBarrier);

        for (int mip = 0; mip <= 4; mip++)
        {
            for (int face = 0; face < 6; face++)
            {

                rhi::RHIDescriptorSet *descriptorSet = descriptorSets[mip * 6 + face];

                cmd->bindComputePipeline(computePipeline);
                cmd->bindComputeDescriptorSet(descriptorSet, 0);
                PrefilterPushConstant pc;
                uint32_t mipSize = std::max(1u, size >> mip);
                pc.face = face;
                pc.faceSize = mipSize;
                pc.roughness = mip / 4.0;
                pc.resolution = 512;
                cmd->setPushConstant(&pc, sizeof(PrefilterPushConstant), 1, true);
                uint32_t groups = (mipSize + 15) / 16;
                cmd->dispatch(groups, groups, 1);
            }
        }

        rhi::TextureBarrier finalTextureBarrier{.texture = cubemapTexture,
                                                .before = rhi::ResourceState::ShaderWrite,
                                                .after = rhi::ResourceState::ShaderRead,
                                                .subresource = initialSubresource};

        cmd->textureBarrier(finalTextureBarrier);
        device->endCommandBuffer(cmd);
        device->destroyComputePipeline(computePipeline);

        for (auto descriptorSet : descriptorSets)
        {
            device->destroyDescriptorSet(descriptorSet);
        }
        device->destroyDescriptorLayout(descriptorLayout);

        return cubemapTexture;
    }

    rhi::RHITexture *generateBrdfLUT(
        std::shared_ptr<rhi::RHIDevice> device, uint32_t size, std::string shaderDir, bool isMetal)
    {
        rhi::TextureDesc brdfLutTextureDesc;
        brdfLutTextureDesc.size = {size, size};
        brdfLutTextureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
        brdfLutTextureDesc.usage = rhi::TextureDesc::Usage::Storage | rhi::TextureDesc::Usage::ShaderRead;

        rhi::RHITexture *brdfLutTexture = device->createTexture(brdfLutTextureDesc);

        std::vector<rhi::RHIDescriptorBinding> binding{
            {rhi::RHIDescriptorBinding::Type::StorageImage, rhi::RHIDescriptorBinding::ShaderStage::Compute, 2}};
        rhi::RHIDescriptorLayout *descriptorLayout = device->createDescriptorLayout(binding);
        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(BRDFPushConstant);
        computePipelineDesc.layouts = {descriptorLayout};
        rhi::ShaderDesc shaderDesc;
        shaderDesc.stage = rhi::ShaderStage::Compute;
        std::string shaderPath = shaderDir + "/brdf-lut/brdf-lut";

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
        rhi::RHIDescriptorSet *descriptorSet = device->createDescriptorSet(descriptorLayout);

        descriptorSet->writeStorageImage(brdfLutTexture, 2, rhi::ImageLayout::General, rhi::TextureSubresource{});
        descriptorSet->commit();

        rhi::RHICommandBuffer *cmd = device->createCommandBuffer();
        rhi::TextureSubresource initialSubresource{};

        rhi::TextureBarrier textureBarrier{.texture = brdfLutTexture,
                                           .before = rhi::ResourceState::Undefined,
                                           .after = rhi::ResourceState::ShaderWrite,
                                           .subresource = initialSubresource};
        cmd->textureBarrier(textureBarrier);
        cmd->bindComputePipeline(computePipeline);
        cmd->bindComputeDescriptorSet(descriptorSet, 0);
        BRDFPushConstant pc;
        pc.faceSize = size;
        cmd->setPushConstant(&pc, sizeof(BRDFPushConstant), 1, true);
        uint32_t groups = (size + 15) / 16;
        cmd->dispatch(groups, groups, 1);

        rhi::TextureBarrier finalTextureBarrier{.texture = brdfLutTexture,
                                                .before = rhi::ResourceState::ShaderWrite,
                                                .after = rhi::ResourceState::ShaderRead,
                                                .subresource = initialSubresource};

        cmd->textureBarrier(finalTextureBarrier);
        device->endCommandBuffer(cmd);
        device->destroyComputePipeline(computePipeline);
        device->destroyDescriptorSet(descriptorSet);
        device->destroyDescriptorLayout(descriptorLayout);

        return brdfLutTexture;
    }

} // namespace nitro::renderer
