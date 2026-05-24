#pragma once
#include <glm/glm.hpp>

namespace nitro::rhi
{
    struct PushConstant
    {
        glm::mat4 model;
    };
}