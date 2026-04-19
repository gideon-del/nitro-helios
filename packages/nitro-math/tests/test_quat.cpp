#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../include/math/quat.h"
using namespace nitro::math;
using Catch::Approx;

TEST_CASE("Quat identity")
{
    Quat q;
    REQUIRE(q.w == Approx(1.0f));
    REQUIRE(q.x == Approx(0.0f));
}

TEST_CASE("Quat fromAxisAngle — 90deg Y rotation")
{
    Quat q = Quat::fromAxisAngle({0, 1, 0}, M_PI / 2.0f);
    REQUIRE(q.length() == Approx(1.0f));
    // Rotate {1,0,0} 90deg around Y should give {0,0,-1}
    Mat4 m = q.toMat4();
    Vec4D result = m * Vec4D{1, 0, 0, 0};
    REQUIRE(result.x == Approx(0.0f).margin(1e-5f));
    REQUIRE(result.z == Approx(-1.0f).margin(1e-5f));
}

TEST_CASE("Quat slerp — t=0 gives q1")
{
    Quat a = Quat::fromAxisAngle({0, 1, 0}, 0.0f);
    Quat b = Quat::fromAxisAngle({0, 1, 0}, 3.14159f / 2.0f);
    Quat result = a.slerp(b, 0.0f);
    REQUIRE(result.w == Approx(a.w).margin(1e-5f));
}

TEST_CASE("Quat slerp — t=1 gives q2")
{
    Quat a = Quat::fromAxisAngle({0, 1, 0}, 0.0f);
    Quat b = Quat::fromAxisAngle({0, 1, 0}, 3.14159f / 2.0f);
    Quat result = a.slerp(b, 1.0f);
    REQUIRE(result.w == Approx(b.w).margin(1e-5f));
}

TEST_CASE("Quat toMat4 — identity Quat gives identity matrix")
{
    Quat q;
    Mat4 m = q.toMat4();
    REQUIRE(m.at(0, 0) == Approx(1.0f));
    REQUIRE(m.at(1, 1) == Approx(1.0f));
    REQUIRE(m.at(2, 2) == Approx(1.0f));
    REQUIRE(m.at(0, 1) == Approx(0.0f));
}