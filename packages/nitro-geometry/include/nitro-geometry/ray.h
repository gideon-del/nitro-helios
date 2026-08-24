#pragma once
#include <glm/glm.hpp>
namespace nitro::geometry
{
    struct Ray
    {
        Ray() {}
        Ray(glm::vec3 origin, glm::vec3 dir) : origin(origin), dir(dir)
        {
            assert(glm::length(dir) > 0.0f);

            dir = glm::normalize(dir);
        }

        glm::vec3 rayEnd() const
        {
            return origin + (dir * tMax);
        }
        glm::vec3 origin = glm::vec3(0.0f);
        glm::vec3 dir = glm::vec3(0.0f, 0.0f, 1.0f);
        float tMin = 0.0f;
        float tMax = 1.0f;
    };
} // namespace nitro::geometry
