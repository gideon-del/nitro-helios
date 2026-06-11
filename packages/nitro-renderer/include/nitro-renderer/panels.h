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

    struct RendererPanel
    {
        void draw(RendererSettings &settings);
    };
} // namespace nitro::renderer
