#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>

namespace nitro::renderer
{
    struct DefaultMaterialTextures
    {
        rhi::RHITexture *baseColor;
        rhi::RHITexture *normal;
        rhi::RHITexture *metallicRoughness;
        rhi::RHITexture *ao;
        rhi::RHITexture *emissive;
    };

    struct Material
    {
        rhi::RHITexture *baseColor = nullptr;
        rhi::RHITexture *normal = nullptr;
        rhi::RHITexture *metallicRoughness = nullptr;
        rhi::RHITexture *ao = nullptr;
        rhi::RHITexture *emissive = nullptr;

        float metallicFactor = 0.0;
        float roughnessFactor = 0.5;
        glm::vec4 baseColorFactor{1.0f};

        rhi::RHIDescriptorSet *descriptorSet;
        bool hasTextures() const
        {
            return baseColor != nullptr;
        }
    };

    struct MaterialDesc
    {
        rhi::RHITexture *baseColor = nullptr;
        rhi::RHITexture *normal = nullptr;
        rhi::RHITexture *metallicRoughness = nullptr;
        rhi::RHITexture *ao = nullptr;
        rhi::RHITexture *emissive = nullptr;

        float metallicFactor = 0.0;
        float roughnessFactor = 0.5;
        glm::vec4 baseColorFactor{1.0f};

        bool hasTextures() const
        {
            return baseColor != nullptr;
        }
    };

    class MaterialSystem
    {
    public:
        MaterialSystem(std::shared_ptr<rhi::RHIDevice> device);
        ~MaterialSystem();

        DefaultMaterialTextures &getDefaults() { return m_defaults; }
        rhi::RHIDescriptorLayout *getMaterialLayout() { return m_materialLayout; };

        std::shared_ptr<Material> getDefaultMaterial();
        std::shared_ptr<Material> createMaterial(MaterialDesc &desc);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        DefaultMaterialTextures m_defaults;
        rhi::RHIDescriptorLayout *m_materialLayout;
        rhi::RHIDescriptorSet *m_materialDescriptorSet;
        std::shared_ptr<Material> m_defaultMaterial;
    };
} // namespace nitro::renderer
