#pragma once
#include "render-object.h"
#include <vector>

namespace nitro::renderer
{
    struct Scene
    {
        std::vector<RenderObject> objects;
        void draw(rhi::RHICommandBuffer *cmd)
        {
            for (auto &obj : objects)
            {
                obj.draw(cmd);
            }
        };
    };
} // namespace nitro::renderer
