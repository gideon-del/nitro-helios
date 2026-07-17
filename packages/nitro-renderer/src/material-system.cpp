#include <nitro-renderer/material-system.h>

namespace nitro::renderer
{
    MaterialSystem::MaterialSystem(std::shared_ptr<rhi::RHIDevice> device) : m_device(device)
    {

        std::vector<rhi::RHIDescriptorBinding> materialBindings = {
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Fragment, 0},
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Fragment, 1},
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Fragment, 2},
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Fragment, 3},
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Fragment, 4},
        };

        m_materialLayout = m_device->createDescriptorLayout(materialBindings);

        auto createSolidTexture = [&](glm::vec4 color)
        {
            rhi::TextureDesc textureDesc;
            textureDesc.usage = rhi::TextureDesc::Usage::ShaderRead;
            textureDesc.size = {1, 1};
            textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;
            uint8_t bytes[4] = {
                uint8_t(color.r * 255.0f),
                uint8_t(color.g * 255.0f),
                uint8_t(color.b * 255.0f),
                uint8_t(color.a * 255.0f)};
            textureDesc.initialData = bytes;
            textureDesc.sampler = rhi::TextureDesc::Sampler::Sampler2D;
            rhi::TextureBinding textureBinding;

            textureBinding.texture = m_device->createTexture(textureDesc);
            textureBinding.sampler = m_device->defaultSamplers().anisotropicRepeat;
            return textureBinding;
        };

        m_defaults.baseTexture = createSolidTexture({1.0f, 0.0f, 0.0f, 1.0f});
        m_defaults.normal = createSolidTexture({0.5f, 0.5f, 1.0f, 1.0f});
        m_defaults.metallicRoughness = createSolidTexture({0.0f, 0.5f, 0.0f, 1.0f});
        m_defaults.ao = createSolidTexture({1.0f, 1.0f, 1.0f, 1.0f});
        m_defaults.emissive = createSolidTexture({0.0f, 0.0f, 0.0f, 1.0f});

        m_materialDescriptorSet = m_device->createDescriptorSet(m_materialLayout);

        m_materialDescriptorSet->writeTexture(m_defaults.baseTexture, 0, rhi::ImageLayout::ShaderReadOnly);
        m_materialDescriptorSet->writeTexture(m_defaults.normal, 1, rhi::ImageLayout::ShaderReadOnly);
        m_materialDescriptorSet->writeTexture(m_defaults.metallicRoughness, 2, rhi::ImageLayout::ShaderReadOnly);
        m_materialDescriptorSet->writeTexture(m_defaults.ao, 3, rhi::ImageLayout::ShaderReadOnly);
        m_materialDescriptorSet->writeTexture(m_defaults.emissive, 4, rhi::ImageLayout::ShaderReadOnly);
        m_materialDescriptorSet->commit();

        m_defaultMaterial = std::make_shared<Material>();

        m_defaultMaterial->textures = m_defaults;
        m_defaultMaterial->descriptorSet = m_materialDescriptorSet;
    };

    MaterialSystem::~MaterialSystem()
    {
        m_device->destroyDescriptorSet(m_materialDescriptorSet);
        m_device->destroyTexture(m_defaults.baseTexture.texture);
        m_device->destroyTexture(m_defaults.normal.texture);
        m_device->destroyTexture(m_defaults.metallicRoughness.texture);
        m_device->destroyTexture(m_defaults.ao.texture);
        m_device->destroyTexture(m_defaults.emissive.texture);

        m_device->destroyDescriptorLayout(m_materialLayout);
    }

    std::shared_ptr<Material> MaterialSystem::getDefaultMaterial()
    {
        return m_defaultMaterial;
    };

    std::shared_ptr<Material> MaterialSystem::createMaterial(const MaterialDesc &desc)
    {
        auto material = std::make_shared<Material>();

        material->textures = desc.textures;
        material->baseColorFactor = desc.baseColorFactor;
        material->metallicFactor = desc.metallicFactor;
        material->roughnessFactor = desc.roughnessFactor;

        if (desc.textures.baseTexture.isValid())
        {
            material->descriptorSet = m_device->createDescriptorSet(m_materialLayout);

            material->descriptorSet->writeTexture(material->textures.baseTexture, 0, rhi::ImageLayout::ShaderReadOnly);

            material->descriptorSet->writeTexture(material->textures.normal.isValid() ? material->textures.normal : m_defaults.normal, 1, rhi::ImageLayout::ShaderReadOnly);
            material->descriptorSet->writeTexture(material->textures.metallicRoughness.isValid() ? material->textures.metallicRoughness : m_defaults.metallicRoughness, 2, rhi::ImageLayout::ShaderReadOnly);
            material->descriptorSet->writeTexture(material->textures.ao.isValid() ? material->textures.ao : m_defaults.ao, 3, rhi::ImageLayout::ShaderReadOnly);
            material->descriptorSet->writeTexture(material->textures.emissive.isValid() ? material->textures.emissive : m_defaults.emissive, 4, rhi::ImageLayout::ShaderReadOnly);

            material->descriptorSet->commit();
        }
        return material;
    };
} // namespace nitro::renderer
