#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>
#include "handles.h"
#include "nitro-renderer/legacy/per-frame.h"
#include "nitro-assets/manager.h"
#include "nitro-renderer/cache/gpu-resource-cache.h"
#include <nitro-rhi/rhi-constants.h>
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

    static constexpr uint32_t kMaxMaterialTextures = 1024;
    struct MaterialManagerFrameResource
    {
        rhi::RHIBuffer *materialBuffer = nullptr;
        size_t capacity = 1024;
        rhi::RHIDescriptorSet *textureSet = nullptr;
    };
    static constexpr uint8_t kAllFrames = (1 << rhi::kMaxFramesInFlight) - 1;
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
        rhi::RHITexture *getTexture(const uint32_t &idx)
        {
            if (idx >= m_textures.size())
                return nullptr;

            return m_textures[idx];
        };
        void flush();
        std::optional<assets::MaterialAssetHandle> getAssetHandle(const GPUMaterialHandle &handle);
        void clear();
        bool isTexturesStale()
        {
            auto frameIdx = m_device->getCurrentFrameIndex();
            auto bit = 1 << frameIdx;
            bool isStale = (m_dirtMaterialTextures & bit) != 0;

            if (isStale)
            {
                std::cout << "Frame Idx " << frameIdx << " is stale " << isStale << std::endl;
            }
            return isStale;
        }
        void markAsNotStale()
        {
            auto frameIdx = m_device->getCurrentFrameIndex();
            auto bit = 1 << frameIdx;

            m_dirtMaterialTextures &= ~bit;
        };
        void markMaterialBufferAsDirty()
        {
            m_dirtMaterialBuffer = kAllFrames;
        }
        void markMaterialTexturesAsDirty()
        {
            m_dirtMaterialTextures = kAllFrames;
            m_dirtyTextureSets = kAllFrames;
        }
        uint32_t
        addTexture(const assets::TextureHandle &handle, rhi::TextureDesc::ImageFormat format);

        rhi::RHIDescriptorLayout *textureLayout() const { return m_textureLayout; }
        rhi::RHIDescriptorSet *textureSet() { return m_resources.current(m_device->getCurrentFrameIndex()).textureSet; }

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::vector<rhi::RHITexture *> m_textures;
        std::vector<GPUMaterial> m_materials;
        PerFrame<MaterialManagerFrameResource> m_resources;
        std::shared_ptr<assets::AssetManager> m_assetManager;
        std::shared_ptr<GPUResourceCache> m_gpuResourceCache;
        std::unordered_map<assets::MaterialAssetHandle, GPUMaterialHandle, assets::MaterialAssetHandleHash> m_assetHandleToGPUHandle;

        uint8_t m_dirtMaterialBuffer = 0;
        uint8_t m_dirtMaterialTextures = 0;
        uint8_t m_dirtyTextureSets = 0;

        rhi::RHIDescriptorLayout *m_textureLayout = nullptr;

    private:
        void buildFrameBuffer(uint32_t frameIdx);
        void writeTextureSet(uint32_t frameIdx);
    };

} // namespace nitro::renderer
