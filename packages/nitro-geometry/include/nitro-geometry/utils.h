#pragma once
#include "ray.h"
#include <glm/glm.hpp>

namespace nitro::geometry
{
    inline bool rayAABB(const Ray &ray, const glm::vec3 &bMin, const glm::vec3 &bMax, float &tHit)
    {
        constexpr float kEps = 1e-8f;
        glm::vec3 d = ray.dir;
        d.x = (std::abs(d.x) < kEps) ? std::copysign(kEps, d.x) : d.x;
        d.y = (std::abs(d.y) < kEps) ? std::copysign(kEps, d.y) : d.y;
        d.z = (std::abs(d.z) < kEps) ? std::copysign(kEps, d.z) : d.z;
        glm::vec3 invDir = 1.0f / d;

        glm::vec3 t0 = (bMin - ray.origin) * invDir;
        glm::vec3 t1 = (bMax - ray.origin) * invDir;

        glm::vec3 tEntry = glm::min(t0, t1);
        glm::vec3 tExit = glm::max(t0, t1);

        float tNear = std::max(std::max(tEntry.x, tEntry.y), tEntry.z);
        float tFar = std::min(std::min(tExit.x, tExit.y), tExit.z);

        if (tNear > tFar || tFar < 0.0f || tNear < 0.0f)
            return false;

        tHit = tNear;

        return tHit <= ray.tMax;
    }
} // namespace nitro::geometry
