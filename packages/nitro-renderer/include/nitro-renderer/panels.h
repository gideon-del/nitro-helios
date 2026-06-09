#pragma once
#include "settings.h"

namespace nitro::renderer
{
    struct LightPanel
    {
        void draw(LightingSettings &settings);
    };
    struct ShadowPanel
    {
        void draw(ShadowSettings &settings);
    };
} // namespace nitro::renderer
