#include "nitro-renderer/material-manager.h"

namespace nitro::renderer
{
    MaterialManager::MaterialManager(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<assets::AssetManager> assetManager, std::shared_ptr<GPUResourceCache> gpuResourceCache) : m_device(device), m_assetManager(std::move(assetManager)), m_gpuResourceCache(std::move(gpuResourceCache))
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
    }

    uint32_t MaterialManager::addTexture(const assets::TextureHandle &handle, rhi::TextureDesc::ImageFormat format)
    {
        auto texture = m_assetManager->getAsset(handle);
        if (!texture)
            return INVALID_TEXTURE_INDEX;

        uint32_t id = static_cast<uint32_t>(m_textures.size());

        auto gpuTexture = m_gpuResourceCache->get(handle, *m_assetManager.get(), format);

        if (!gpuTexture)
            return INVALID_TEXTURE_INDEX;
        m_textures.push_back(gpuTexture);
        return id;
    }

    GPUMaterialHandle MaterialManager::addMaterial(const assets::MaterialAssetHandle &handle)
    {

        auto material = m_assetManager->getAsset(handle);
        if (!material)
        {
            return {};
        };
        if (auto it = m_assetHandleToGPUHandle.find(handle); it != m_assetHandleToGPUHandle.end())
        {
            return it->second;
        }

        GPUMaterial gpuMaterial;
        gpuMaterial.assetHandle = handle;
        gpuMaterial.textures.albedo = addTexture(material->textures.albedo, rhi::TextureDesc::ImageFormat::ColorSRGB8);

        gpuMaterial.textures.normalMap = addTexture(material->textures.normalMap, rhi::TextureDesc::ImageFormat::ColorRGBA8);

        gpuMaterial.textures.metallicRoughness = addTexture(material->textures.metallicRoughness, rhi::TextureDesc::ImageFormat::ColorRGBA8);

        gpuMaterial.textures.occlusionMap = addTexture(material->textures.occlusionMap, rhi::TextureDesc::ImageFormat::ColorRGBA8);

        gpuMaterial.textures.emissive = addTexture(material->textures.emissive, rhi::TextureDesc::ImageFormat::ColorSRGB8);

        gpuMaterial.parameters.albedo = material->parameters.albedo;
        gpuMaterial.parameters.metallic = material->parameters.metallic;
        gpuMaterial.parameters.roughness = material->parameters.roughness;

        HandleValueType id = static_cast<HandleValueType>(m_materials.size());

        m_materials.push_back(std::move(gpuMaterial));
        GPUMaterialHandle gpuHandle{id};
        m_assetHandleToGPUHandle[handle] = gpuHandle;
        markMaterialBufferAsDirty();
        markMaterialTexturesAsDirty();
        return gpuHandle;
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

            desc.size = sizeof(GPUMaterialDesc) * resource.capacity;
            desc.storage = rhi::BufferDesc::StorageMode::Dynamic;
            desc.usage = rhi::BufferDesc::Usage::Storage;

            resource.materialBuffer = m_device->createBuffer(desc);
        }
        std::vector<GPUMaterialDesc> descriptors;
        descriptors.reserve(m_materials.size());

        for (auto &material : m_materials)
        {
            GPUMaterialDesc desc;
            desc.textures = material.textures;
            desc.parameters = material.parameters;
            descriptors.push_back(desc);
        }
        resource.materialBuffer->upload(
            descriptors.data(),
            sizeof(GPUMaterialDesc) * descriptors.size());
    };

    GPUMaterial *MaterialManager::getMaterial(const GPUMaterialHandle &handle)
    {
        if (!handle.isValid() || handle.index >= m_materials.size())
        {
            return nullptr;
        }

        return &m_materials[handle.index];
    };

    std::optional<assets::MaterialAssetHandle> MaterialManager::getAssetHandle(const GPUMaterialHandle &handle)
    {
        auto it = getMaterial(handle);
        if (!it)
            return std::nullopt;

        return it->assetHandle;
    }

    void MaterialManager::clear()
    {

        m_materials.clear();
        m_assetHandleToGPUHandle.clear();
        m_gpuResourceCache->clear();
    }
} // namespace nitro::renderer
