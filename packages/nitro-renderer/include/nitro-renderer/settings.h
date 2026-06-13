#pragma once
#include <glm/glm.hpp>
#include <nitro-geometry/camera.h>

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
        DirectionalLight = 7
    };
    struct PointLight
    {
        glm::vec4 position{5.0f, 5.0f, 5.0f, 1.0f};
        glm::vec4 color{1.0f, 0.0f, 1.0f, 1.0f};
        float radius = 20.0f;
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

        glm::vec3 lightColor = glm::vec3(1.0f);
        geometry::OrbitalCamera lightCamera;
        std::vector<PointLight> pointLights;
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
        Deferred
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
    struct RendererSettings
    {
        ShadowSettings shadow;
        LightingSettings light;
        RendererType renderer = RendererType::Forward;
        StatSettings stats;
        DebugMode selectedDebugMode = DebugMode::Lit;
    };

} // namespace nitro::renderer
