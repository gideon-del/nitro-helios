#pragma once
#include <glm/glm.hpp>

namespace nitro::geometry
{
    struct PushConstant
    {
        glm::mat4 model;
        glm::mat4 normalMatrix;

        void applyNormalMatrix()
        {
            normalMatrix = glm::transpose(glm::inverse(model));
        }
    };
}