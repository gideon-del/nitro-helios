#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../include/math/utils.h"

using namespace nitro::math;
using Catch::Approx;

// utils tests
TEST_CASE("lerp scalar")
{
    REQUIRE(lerp(0.0f, 10.0f, 0.5f) == Approx(5.0f));
    REQUIRE(lerp(0.0f, 10.0f, 0.0f) == Approx(0.0f));
    REQUIRE(lerp(0.0f, 10.0f, 1.0f) == Approx(10.0f));
}

TEST_CASE("clamp")
{
    REQUIRE(clamp(5.0f, 0.0f, 1.0f) == Approx(1.0f));
    REQUIRE(clamp(-1.0f, 0.0f, 1.0f) == Approx(0.0f));
    REQUIRE(clamp(0.5f, 0.0f, 1.0f) == Approx(0.5f));
}

TEST_CASE("smoothstep")
{
    REQUIRE(smoothstep(0.0f, 1.0f, 0.0f) == Approx(0.0f));
    REQUIRE(smoothstep(0.0f, 1.0f, 1.0f) == Approx(1.0f));
    REQUIRE(smoothstep(0.0f, 1.0f, 0.5f) == Approx(0.5f));
}

TEST_CASE("toRadians and toDegrees")
{
    REQUIRE(toRadians(180.0f) == Approx(PI));
    REQUIRE(toDegrees(PI) == Approx(180.0f));
}