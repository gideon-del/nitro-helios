#include "nitro-renderer/material-manager.h"

namespace nitro::renderer
{
    MaterialManager::MaterialManager(std::shared_ptr<rhi::RHIDevice> device) : m_device(device) {}

    MaterialManager::~MaterialManager()
    {
        if (m_materialBuffer)
        {
            m_device->destroyBuffer(m_materialBuffer);
        }
        for (auto &texture : m_textures)
        {
            m_device->destroyTexture(texture);
        }
    }

    uint32_t MaterialManager::addTexture(rhi::RHITexture *texture)
    {
        if (!texture)
            return INVALID_TEXTURE_INDEX;

        uint32_t id = static_cast<uint32_t>(m_textures.size());

        m_textures.push_back(texture);
        return id;
    }

    uint32_t MaterialManager::addMaterial(const MaterialDesc &desc)
    {
        Material material;

        material.textures.albedo = addTexture(desc.textures.albedo);
        material.textures.normalMap = addTexture(desc.textures.normalMap);
        material.textures.metallicRoughness = addTexture(desc.textures.metallicRoughness);
        material.textures.occlusionMap = addTexture(desc.textures.occlusionMap);
        material.textures.emissive = addTexture(desc.textures.emissive);

        material.parameters.albedo = desc.parameters.albedo;
        material.parameters.metallic = desc.parameters.metallic;
        material.parameters.roughness = desc.parameters.roughness;

        uint32_t id = static_cast<uint32_t>(m_materials.size());

        m_materials.push_back(std::move(material));

        return id;
    };

    void MaterialManager::buildMegaMaterialBuffer()
    {
        rhi::BufferDesc desc;
        desc.initialData = m_materials.data();
        desc.size = sizeof(Material) * m_materials.size();
        desc.storage = rhi::BufferDesc::StorageMode::GPU;
        desc.usage = rhi::BufferDesc::Usage::Storage;

        if (m_materialBuffer)
        {
            m_device->destroyBuffer(m_materialBuffer);
        }
        m_materialBuffer = m_device->createBuffer(desc);
    };
} // namespace nitro::renderer
