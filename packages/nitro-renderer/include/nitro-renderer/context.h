#pragma once
#include "scene.h"
#include <nitro-geometry/camera.h>

namespace nitro::renderer
{
    struct RenderContext
    {
        Scene *scene;
        geometry::OrbitalCamera *camera;
        float CAMERA_NEAR = 0.1f;
        float CAMERA_FAR = 100.0f;
    };
} // namespace nitro::renderer
