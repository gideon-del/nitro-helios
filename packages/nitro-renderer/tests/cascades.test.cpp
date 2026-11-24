#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "nitro-renderer/render-context.h"
#include "nitro-renderer/shadows/cascades.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

using namespace nitro::renderer;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{
    CameraData makeCamera(glm::vec3 pos, float yawDeg)
    {
        CameraData c{};
        glm::vec3 fwd(std::sin(glm::radians(yawDeg)), -0.2f, -std::cos(glm::radians(yawDeg)));
        c.view = glm::lookAt(pos, pos + glm::normalize(fwd), glm::vec3(0, 1, 0));
        c.fovY = glm::radians(60.0f);
        c.aspect = 16.0f / 9.0f;
        c.nearPlane = 0.1f;
        c.farPlane = 200.0f;
        return c;
    }
    const glm::vec3 kSun = glm::normalize(glm::vec3(-0.26f, -0.86f, -0.43f));
    PipelineSettings::Shadow settings() { return {.lambda = 0.8f, .resolution = 2048, .casterMargin = 50.0f}; }
    float radiusOf(const glm::mat4 &vp) { return 1.0f / glm::length(glm::vec3(vp[0][0], vp[1][0], vp[2][0])); }
}

TEST_CASE("Cascade splits increase and end at far", "[cascades]")
{
    auto cam = makeCamera({0, 2, 0}, 0);
    auto r = computeCascades(cam, kSun, settings());
    float prev = cam.nearPlane;
    for (uint32_t i = 0; i < kCascadeCount; ++i)
    {
        CHECK(r.splits[i] > prev);
        prev = r.splits[i];
    }
    CHECK_THAT(r.splits[kCascadeCount - 1], WithinRel(cam.farPlane));
}

TEST_CASE("Cascades are deterministic", "[cascades]")
{
    auto cam = makeCamera({3, 2, -5}, 30);
    auto a = computeCascades(cam, kSun, settings());
    auto b = computeCascades(cam, kSun, settings());
    for (uint32_t i = 0; i < kCascadeCount; ++i)
        CHECK(a.viewProj[i] == b.viewProj[i]);
}

TEST_CASE("Cascade radius ignores camera rotation", "[cascades]")
{
    auto a = computeCascades(makeCamera({0, 2, 0}, 0), kSun, settings());
    auto b = computeCascades(makeCamera({0, 2, 0}, 73), kSun, settings());
    for (uint32_t i = 0; i < kCascadeCount; ++i)
        CHECK_THAT(radiusOf(a.viewProj[i]), WithinRel(radiusOf(b.viewProj[i]), 1e-5f));
}

TEST_CASE("Cascades move only by whole texels", "[cascades]")
{
    auto s = settings();
    auto a = computeCascades(makeCamera({0, 2, 0}, 20), kSun, s);
    auto b = computeCascades(makeCamera({1.37f, 2, -0.61f}, 20), kSun, s);
    for (uint32_t i = 0; i < kCascadeCount; ++i)
    {
        glm::vec4 pa = a.viewProj[i] * glm::vec4(0, 0, 0, 1);
        glm::vec4 pb = b.viewProj[i] * glm::vec4(0, 0, 0, 1);
        float texelsX = (pb.x - pa.x) * 0.5f * float(s.resolution);
        float texelsY = (pb.y - pa.y) * 0.5f * float(s.resolution);
        CHECK_THAT(texelsX, WithinAbs(std::round(texelsX), 1e-2));
        CHECK_THAT(texelsY, WithinAbs(std::round(texelsY), 1e-2));
    }
}

TEST_CASE("Slice corners lie inside their cascade's clip volume", "[cascades]")
{
    auto cam = makeCamera({3, 2, -5}, 45);
    auto r = computeCascades(cam, kSun, settings());
    glm::mat4 inv = glm::inverse(cam.view);
    float t = std::tan(cam.fovY * 0.5f), prevFar = cam.nearPlane;
    for (uint32_t i = 0; i < kCascadeCount; ++i)
    {
        INFO("cascade " << i);
        for (float z : {prevFar, r.splits[i]})
            for (float sx : {-1.f, 1.f})
                for (float sy : {-1.f, 1.f})
                {
                    glm::vec4 c = r.viewProj[i] * (inv * glm::vec4(sx * t * z * cam.aspect, sy * t * z, -z, 1));
                    CHECK(std::abs(c.x) <= 1.0f + 1e-4f);
                    CHECK(std::abs(c.y) <= 1.0f + 1e-4f);
                    CHECK(c.z >= -1e-4f);
                    CHECK(c.z <= 1.0f + 1e-4f);
                }
        prevFar = r.splits[i];
    }
}