#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>
#include "handles.h"
#include "per-frame.h"
#include "nitro-assets/manager.h"
#include "gpu-resource-cache.h"
namespace nitro::renderer
{

    static constexpr uint32_t INVALID_TEXTURE_INDEX = 0xFFFFFFFFu;
    struct MaterialTextures
    {
        uint32_t albedo = INVALID_TEXTURE_INDEX;
        uint32_t normalMap = INVALID_TEXTURE_INDEX;
        uint32_t metallicRoughness = INVALID_TEXTURE_INDEX;
        uint32_t occlusionMap = INVALID_TEXTURE_INDEX;
        uint32_t emissive = INVALID_TEXTURE_INDEX;
        float _pads[3];
    };

    struct MaterialParameters
    {

        glm::vec4 albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        float metallic = 0.0f;
        float roughness = 0.5f;
        float _pads[2];
    };

    struct GPUMaterial
    {
        MaterialTextures textures;
        MaterialParameters parameters;
        assets::MaterialAssetHandle assetHandle;
    };
    struct GPUMaterialDesc
    {
        MaterialTextures textures;
        MaterialParameters parameters;
    };

    struct MaterialManagerFrameResource
    {
        rhi::RHIBuffer *materialBuffer = nullptr;

        size_t capacity = 1024;
    };
    class MaterialManager
    {
    public:
        MaterialManager(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<assets::AssetManager> assetManager, std::shared_ptr<GPUResourceCache> gpuResourceCache);
        ~MaterialManager();
        GPUMaterialHandle addMaterial(const assets::MaterialAssetHandle &handle);
        const std::vector<rhi::RHITexture *> &getTextures() { return m_textures; }
        const std::vector<GPUMaterial> &getMaterials() { return m_materials; }
        rhi::RHIBuffer *getMaterialBuffer() { return m_resources.current(m_device->getCurrentFrameIndex()).materialBuffer; }
        void buildMegaMaterialBuffer();
        GPUMaterial *getMaterial(const GPUMaterialHandle &handle);
        void flush();
        std::optional<assets::MaterialAssetHandle> getAssetHandle(const GPUMaterialHandle &handle);
        void clear();
        bool isTexturesStale()
        {
            auto frameIdx = m_device->getCurrentFrameIndex();
            auto bit = 1 << frameIdx;
            return (m_dirtMaterialTextures & bit) != 0;
        }
        void markAsNotStale()
        {
            auto frameIdx = m_device->getCurrentFrameIndex();
            auto bit = 1 << frameIdx;
            m_dirtMaterialTextures &= ~bit;
        };

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::vector<rhi::RHITexture *> m_textures;
        std::vector<GPUMaterial> m_materials;
        PerFrame<MaterialManagerFrameResource> m_resources;
        std::shared_ptr<assets::AssetManager> m_assetManager;
        std::shared_ptr<GPUResourceCache> m_gpuResourceCache;
        std::unordered_map<assets::MaterialAssetHandle, GPUMaterialHandle, assets::MaterialAssetHandleHash> m_assetHandleToGPUHandle;
        uint32_t
        addTexture(const assets::TextureHandle &handle, rhi::TextureDesc::ImageFormat format);
        uint8_t m_dirtMaterialBuffer = 0;
        uint8_t m_dirtMaterialTextures = 0;
        void buildFrameBuffer(uint32_t frameIdx);
        void markMaterialBufferAsDirty()
        {
            m_dirtMaterialBuffer = (1 << g_MAX_FRAMES_IN_FLIGHT) - 1;
        }
        void markMaterialTexturesAsDirty()
        {
            m_dirtMaterialTextures = (1 << g_MAX_FRAMES_IN_FLIGHT) - 1;
        }
    };

} // namespace nitro::renderer
