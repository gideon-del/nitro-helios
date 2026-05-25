#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
namespace nitro::rhi
{
    struct GlobalTransformation
    {
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));
        ;
        glm::mat4 proj = glm::mat4(1.0f);
    };

} // namespace nitro::rhi
