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

            return m_device->createTexture(textureDesc);
        };

        m_defaults.baseColor = createSolidTexture({1.0f, 0.0f, 0.0f, 1.0f});
        m_defaults.normal = createSolidTexture({0.5f, 0.5f, 1.0f, 1.0f});
        m_defaults.metallicRoughness = createSolidTexture({0.0f, 0.5f, 0.0f, 1.0f});
        m_defaults.ao = createSolidTexture({1.0f, 1.0f, 1.0f, 1.0f});
        m_defaults.emissive = createSolidTexture({0.0f, 0.0f, 0.0f, 1.0f});

        m_materialDescriptorSet = m_device->createDescriptorSet(m_materialLayout);

        m_materialDescriptorSet->writeTexture(m_defaults.baseColor, 0, rhi::ImageLayout::ShaderReadOnly);
        m_materialDescriptorSet->writeTexture(m_defaults.normal, 1, rhi::ImageLayout::ShaderReadOnly);
        m_materialDescriptorSet->writeTexture(m_defaults.metallicRoughness, 2, rhi::ImageLayout::ShaderReadOnly);
        m_materialDescriptorSet->writeTexture(m_defaults.ao, 3, rhi::ImageLayout::ShaderReadOnly);
        m_materialDescriptorSet->writeTexture(m_defaults.emissive, 4, rhi::ImageLayout::ShaderReadOnly);
        m_materialDescriptorSet->commit();

        m_defaultMaterial = std::make_shared<Material>();

        m_defaultMaterial->baseColor = m_defaults.baseColor;
        m_defaultMaterial->normal = m_defaults.normal;
        m_defaultMaterial->ao = m_defaults.ao;
        m_defaultMaterial->metallicRoughness = m_defaults.metallicRoughness;
        m_defaultMaterial->emissive = m_defaults.emissive;
        m_defaultMaterial->descriptorSet = m_materialDescriptorSet;
    };

    MaterialSystem::~MaterialSystem()
    {
        m_device->destroyDescriptorSet(m_materialDescriptorSet);
        m_device->destroyTexture(m_defaults.baseColor);
        m_device->destroyTexture(m_defaults.normal);
        m_device->destroyTexture(m_defaults.metallicRoughness);
        m_device->destroyTexture(m_defaults.ao);
        m_device->destroyTexture(m_defaults.emissive);
        m_device->destroyDescriptorLayout(m_materialLayout);
    }

    std::shared_ptr<Material> MaterialSystem::getDefaultMaterial()
    {
        return m_defaultMaterial;
    };

    std::shared_ptr<Material> MaterialSystem::createMaterial(MaterialDesc &desc)
    {
        auto material = std::make_shared<Material>();

        material->baseColor = desc.baseColor;
        material->normal = desc.normal;
        material->metallicRoughness = desc.metallicRoughness;
        material->ao = desc.ao;
        material->emissive = desc.emissive;
        material->baseColorFactor = desc.baseColorFactor;
        material->metallicFactor = desc.metallicFactor;
        material->roughnessFactor = desc.roughnessFactor;

        material->descriptorSet = m_device->createDescriptorSet(m_materialLayout);

        // material->descriptorSet->writeTexture(material->baseColor != nullptr ? material->baseColor : m_defaults.baseColor, 0, rhi::ImageLayout::ShaderReadOnly);
        // material->descriptorSet->writeTexture(material->normal != nullptr ? material->normal : m_defaults.normal, 1, rhi::ImageLayout::ShaderReadOnly);
        // material->descriptorSet->writeTexture(material->metallicRoughness != nullptr ? material->metallicRoughness : m_defaults.metallicRoughness, 2, rhi::ImageLayout::ShaderReadOnly);
        // material->descriptorSet->writeTexture(material->ao != nullptr ? material->ao : m_defaults.ao, 3, rhi::ImageLayout::ShaderReadOnly);
        // material->descriptorSet->writeTexture(material->emissive != nullptr ? material->emissive : m_defaults.emissive, 4, rhi::ImageLayout::ShaderReadOnly);
        material->descriptorSet->writeTexture(material->baseColor, 0, rhi::ImageLayout::ShaderReadOnly);
        material->descriptorSet->writeTexture(material->normal, 1, rhi::ImageLayout::ShaderReadOnly);
        material->descriptorSet->writeTexture(material->metallicRoughness, 2, rhi::ImageLayout::ShaderReadOnly);
        material->descriptorSet->writeTexture(material->ao, 3, rhi::ImageLayout::ShaderReadOnly);
        material->descriptorSet->writeTexture(material->emissive, 4, rhi::ImageLayout::ShaderReadOnly);

        material->descriptorSet->commit();

        return material;
    };
} // namespace nitro::renderer
