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
    struct StatPanel
    {
        void draw(StatSettings &settings);
    };
} // namespace nitro::renderer
