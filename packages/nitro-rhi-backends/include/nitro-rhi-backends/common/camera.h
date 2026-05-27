#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace nitro::rhi
{
    struct OrbitalCamera
    {
        float theta = glm::radians(45.0f);
        float phi = glm::radians(60.0f);
        float radius = 10.0f;

        glm::vec3 target{0.0f, 0.0f, 0.0f};

        glm::vec3 getEye()
        {
            glm::vec3 eye;

            eye.x = target.x + radius * std::sin(theta) * std::cos(phi);
            eye.y = target.y + radius * std::cos(theta);
            eye.z = target.z + radius * std::sin(theta) * std::sin(phi);

            return eye;
        }

        glm::mat4 getView()
        {
            return glm::lookAt(getEye(), target, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        void onMouseMove(float dx, float dy)
        {
            float sensitivity = 0.01;
            phi -= dx * sensitivity;
            theta -= dy * sensitivity;
            theta = glm::clamp(theta, 0.1f, glm::pi<float>() - 0.1f);
        }

        void onScroll(float delta)
        {
            float sensitivity = 0.01;
            radius -= delta * sensitivity;
            radius = glm::max(radius, 0.5f);
        }
    };
}