#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>
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

    struct Material
    {
        MaterialTextures textures;
        MaterialParameters parameters;
    };

    struct MaterialDesc
    {
        struct Textures
        {
            rhi::RHITexture *albedo = nullptr;
            rhi::RHITexture *normalMap = nullptr;
            rhi::RHITexture *metallicRoughness = nullptr;
            rhi::RHITexture *occlusionMap = nullptr;
            rhi::RHITexture *emissive = nullptr;
        };

        Textures textures;

        MaterialParameters parameters;
    };

    class MaterialManager
    {
    public:
                MaterialManager(std::shared_ptr<rhi::RHIDevice> device);
        ~MaterialManager();
        uint32_t addMaterial(const MaterialDesc &desc);
        const std::vector<rhi::RHITexture *> &getTextures() { return m_textures; }
        const std::vector<Material> &getMaterials() { return m_materials; }
        rhi::RHIBuffer *getMaterialBuffer() { return m_materialBuffer; }
        void buildMegaMaterialBuffer();

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::vector<rhi::RHITexture *> m_textures;
        std::vector<Material> m_materials;
        rhi::RHIBuffer *m_materialBuffer = nullptr;
        uint32_t addTexture(rhi::RHITexture *texture);
    };

} // namespace nitro::renderer
