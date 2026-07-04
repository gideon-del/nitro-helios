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

        static std::vector<RenderObject> loadGltfScene(std::string filePath, std::shared_ptr<rhi::RHIDevice> device);
    };
} // namespace nitro::renderer
