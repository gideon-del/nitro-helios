// nitro-renderer/stage-outputs.h
#pragma once
#include "nitro-frame/resources/resource-types.h"

namespace nitro::renderer
{
    // Published by a pipeline's declare(), read by post-process and the renderer.
    struct SceneColor
    {
        frame::LogicalTextureHandle texture;
    };

}