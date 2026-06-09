#pragma once
#include <glm/glm.hpp>
#include <nitro-geometry/camera.h>

namespace nitro::renderer
{

    struct LightingSettings
    {
        float ambient = 0.3f;
        float Ka = 1.0f;
        float Kd = 0.8f;
        float Ks = 0.9f;
        float shininess = 32.0f;

        glm::vec3 lightColor = glm::vec3(1.0f);
        geometry::OrbitalCamera lightCamera;
    };

    struct ShadowSettings
    {
        float bias = 0.005f;
        float normalBias = 0.05f;
        float lambda = 0.5f;
        bool showCascadeColors = false;
    };

    struct RendererSettings
    {
        ShadowSettings shadow;
        LightingSettings light;
    };

} // namespace nitro::renderer
