#pragma once
#include "settings.h"
#include "particle-emitter-system.h"
#include <nitro-rhi/rhi.h>
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
    struct ToneMapPanel
    {
        void draw(ToneMapSettings &settings);
    };
    struct BloomPanel
    {
        void draw(BloomSettings &settings);
    };
    struct ColorGradingPanel
    {
        void draw(ColorGradingSettings &settings);
    };
    struct SSAOPanel
    {
        void draw(SSAOSettings &settings);
    };

    struct EmitterPanel
    {
        void draw(ParticleEmitterSystem &system, rhi::RHIBuffer *emitterBuffer);
    };
} // namespace nitro::renderer
