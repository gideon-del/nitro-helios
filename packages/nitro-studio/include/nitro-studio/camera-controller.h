#pragma once
#include "nitro-geometry/camera.h"
#include <glm/glm.hpp>

namespace nitro::studio
{
    struct CameraInput
    {
        glm::vec2 mouseDelta{0.0f};
        float scroll = 0.0f;
        bool orbit = false;
        glm::vec2 move{0.0f};
        bool enabled = false;
    };
    class CameraController
    {
    public:
        void update(geometry::OrbitalCamera &camera, const CameraInput &input, float dt);
        float moveSpeed = 10.0f;
        float orbitSensitivity = 0.01f;
        float zoomSensitivity = 1.0f;
    };
} // namespace nitro::studio
