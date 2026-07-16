#pragma once
#include <glm/glm.hpp>
#include <nitro-geometry/camera.h>
#include <nitro-renderer/mesh-renderer.h>
namespace nitro::renderer
{

    enum class DebugMode
    {
        Lit = 0,
        Albedo = 1,
        Normal = 2,
        Depth = 3,
        WorldPosition = 4,
        CascadeColor = 5,
        PointLight = 6,
        DirectionalLight = 7,
        HeatMap = 8,
        DistributionGGX = 9,
        FresnelSchlick = 10,
        GeometrySmith = 11,
        Ambient = 12,
        SpecularIBL = 13,
    };

    enum class LightMode
    {
        BlinnPhong = 0,
        LambertDiffuse = 1,
        CookTorrenceStub = 2,
    };

    enum class RendererScenes
    {
        Main = 0,
        PBRGrid = 1,
        DamageHelmet = 2,
    };
    struct PointLight
    {
        glm::vec4 position{5.0f, 5.0f, 5.0f, 1.0f};
        glm::vec4 color{1.0f, 0.0f, 1.0f, 1.0f};
        float radius = 200.0f;
        float intensity = 1.0f;
        float pad[2];
    };
    struct LightingSettings
    {
        float ambient = 0.3f;
        float Ka = 1.0f;
        float Kd = 0.8f;
        float Ks = 0.9f;
        float shininess = 32.0f;
        float roughness = 0.0f;
        float metallic = 0.5f;
        glm::vec3 lightColor = glm::vec3(1.0f);
        geometry::OrbitalCamera lightCamera;
        std::vector<PointLight> pointLights;
        std::shared_ptr<MeshRenderer> pointLightRenderer;
    };

    struct ShadowSettings
    {
        float bias = 0.005f;
        float normalBias = 0.05f;
        float lambda = 0.5f;
        bool showCascadeColors = false;
    };

    enum class RendererType
    {
        Forward,
        Deferred,
        TiledDeferred
    };

    struct BloomSettings
    {
        float threshold = 0.4;
        float intensity = 0.5;
        bool enable = true;
    };
    struct StatSettings
    {
        float fps;
        float frameTime;
        uint32_t drawCalls;
        uint32_t vertices;
        uint32_t triangles;
        std::string renderer;
        std::string backend;
    };

    enum class ToneMapMode
    {
        Linear = 0,
        Reinhard = 1,
        ACES = 2,
    };
    struct ToneMapSettings
    {
        float exposure;
        ToneMapMode mode;
    };
    struct RendererSettings
    {
        ShadowSettings shadow;
        LightingSettings light;
        RendererType renderer = RendererType::TiledDeferred;
        StatSettings stats;
        ToneMapSettings tonemap;
        BloomSettings bloom;
        DebugMode selectedDebugMode = DebugMode::Lit;
        LightMode selectedLightMode = LightMode::BlinnPhong;
        RendererScenes selectedScene = RendererScenes::DamageHelmet;
    };

} // namespace nitro::renderer
