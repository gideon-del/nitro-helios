#pragma once
#include <glm/glm.hpp>

namespace nitro::geometry
{
    struct BlinnPhongLight
    {
        // BlinnPhongLight(glm::vec4 lightPosition) : lightPosition(lightPosition) {};

        glm::vec4 lightPosition;
        glm::vec4 lightColor = glm::vec4(1.0f);
        float ambient = 0.3f;
        float Ka = 1.0f;
        float Kd = 0.8f;
        float Ks = 0.9f;
        float shininess = 120.0f;
    };
} // namespace nitro::geometry
