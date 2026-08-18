#pragma once
#include "scene.h"
#include <nitro-geometry/camera.h>
#include <random>

namespace nitro::renderer
{
    struct SSAOSamples
    {
        std::vector<glm::vec4> samples;
        void generate(uint32_t count = 8)
        {
            std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
            std::default_random_engine generator;

            for (uint32_t i = 0; i < count; i++)
            {
                float scale = float(i / count);
                scale = glm::mix(0.1, 1.0, scale * scale);

                glm::vec3 sample{
                    randomFloats(generator) * 2.0f - 1.0f,
                    randomFloats(generator) * 2.0f - 1.0f,
                    randomFloats(generator)};
                sample = glm::normalize(sample);
                sample *= scale;

                samples.push_back(glm::vec4(sample, 1.0));
            }
        };
    };

    struct RenderContext
    {
        Scene *scene;
        geometry::OrbitalCamera *camera;
        SSAOSamples ssaoSamples;
        float CAMERA_NEAR = 0.1f;
        float CAMERA_FAR = 900.0f;
        float currentTime = 0.0f;
        double lastFrameTime = 0.0;
        double deltaTime = 0.001;
    };
} // namespace nitro::renderer
