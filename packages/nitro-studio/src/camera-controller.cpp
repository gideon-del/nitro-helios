#include "nitro-studio/camera-controller.h"

namespace nitro::studio
{
    void CameraController::update(geometry::OrbitalCamera &camera, const CameraInput &input, float dt)
    {
        if (!input.enabled)
            return;
        if (input.scroll != 0.0f)
        {

            camera.onScroll(input.scroll, zoomSensitivity);
        }
        if (input.orbit)
        {
            camera.onMouseMove(input.mouseDelta.x, input.mouseDelta.y, orbitSensitivity);
        }

        if (input.move.y != 0)
        {
            camera.moveForward(input.move.y * moveSpeed * dt);
        }
        if (input.move.x != 0)
        {
            camera.moveRight(input.move.x * moveSpeed * dt);
        }
    }
} // namespace nitro::studio
