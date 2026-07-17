#pragma once
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>
#include <optional>

namespace nitro::renderer
{

    struct MaterialTextures
    {
        rhi::TextureBinding baseTexture;
        rhi::TextureBinding normal;
        rhi::TextureBinding metallicRoughness;
        rhi::TextureBinding ao;
        rhi::TextureBinding emissive;
    };

    struct Material
    {
        MaterialTextures textures;

        float metallicFactor = 0.0;
        float roughnessFactor = 0.5;
        glm::vec4 baseColorFactor{1.0f};

        rhi::RHIDescriptorSet *descriptorSet;
        bool hasTextures() const
        {
            return textures.baseTexture.isValid();
        }
    };

    struct MaterialDesc
    {
        MaterialTextures textures;
        float metallicFactor = 0.0;
        float roughnessFactor = 0.5;
        glm::vec4 baseColorFactor{1.0f};

        bool hasTextures() const
        {
            return textures.baseTexture.isValid();
        }
    };

    class MaterialSystem
    {
    public:
        MaterialSystem(std::shared_ptr<rhi::RHIDevice> device);
        ~MaterialSystem();

        const MaterialTextures &getDefaults() const { return m_defaults; }
        rhi::RHIDescriptorLayout *getMaterialLayout() { return m_materialLayout; };

        std::shared_ptr<Material> getDefaultMaterial();
        std::shared_ptr<Material> createMaterial(const MaterialDesc &desc);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        MaterialTextures m_defaults;
        rhi::RHIDescriptorLayout *m_materialLayout;
        rhi::RHIDescriptorSet *m_materialDescriptorSet;
        std::shared_ptr<Material> m_defaultMaterial;
    };
} // namespace nitro::renderer
