#include "nitro-renderer/material-manager.h"

namespace nitro::renderer
{
    MaterialManager::MaterialManager(std::shared_ptr<rhi::RHIDevice> device) : m_device(device)
    {
        m_resources.create(
            g_MAX_FRAMES_IN_FLIGHT,
            [](uint32_t frameIdx)
            {
                return MaterialManagerFrameResource{};
            });
    }

    MaterialManager::~MaterialManager()
    {
        for (auto &resource : m_resources)
        {
            m_device->destroyBuffer(resource.materialBuffer);
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

    MaterialHandle MaterialManager::addMaterial(const MaterialDesc &desc)
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

        HandleValueType id = static_cast<HandleValueType>(m_materials.size());

        m_materials.push_back(std::move(material));
        markMaterialBufferAsDirty();
        return MaterialHandle{id};
    };

    void MaterialManager::flush()
    {
        auto frameIdx = m_device->getCurrentFrameIndex();
        uint8_t bit = 1 << frameIdx;

        if (m_dirtMaterialBuffer & bit)
        {
            buildFrameBuffer(frameIdx);
            m_dirtMaterialBuffer &= ~bit;
        }
    }

    void MaterialManager::buildMegaMaterialBuffer()
    {

        for (uint i = 0; i < g_MAX_FRAMES_IN_FLIGHT; i++)
        {
            buildFrameBuffer(i);
        }
        m_dirtMaterialBuffer = 0;
    };

    void MaterialManager::buildFrameBuffer(uint32_t frameIdx)
    {

        auto &resource = m_resources.current(frameIdx);

        size_t needed = m_materials.size();

        if (!resource.materialBuffer || needed > resource.capacity)
        {
            if (resource.materialBuffer)
                m_device->destroyBuffer(resource.materialBuffer);
            if (needed > resource.capacity)
                resource.capacity = std::max((needed * 2), size_t(1024));
            rhi::BufferDesc desc;

            desc.size = sizeof(Material) * resource.capacity;
            desc.storage = rhi::BufferDesc::StorageMode::Dynamic;
            desc.usage = rhi::BufferDesc::Usage::Storage;

            resource.materialBuffer = m_device->createBuffer(desc);
        }

        resource.materialBuffer->upload(
            m_materials.data(),
            sizeof(Material) * m_materials.size());
    };

    Material *MaterialManager::getMaterial(const MaterialHandle &handle)
    {
        if (!handle.isValid() || handle.id >= m_materials.size())
        {
            return nullptr;
        }

        return &m_materials[handle.id];
    };
} // namespace nitro::renderer
