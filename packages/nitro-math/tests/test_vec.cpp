#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../include/math/vec.h"

using namespace nitro::math;
using Catch::Approx;
TEST_CASE("Vec2 dot product")
{
    REQUIRE(Vec2D{1, 0}.dot(Vec2D{0, 1}) == Approx(0.0f));
    REQUIRE(Vec2D{3, 0}.dot(Vec2D{5, 0}) == Approx(15.0f));
}

TEST_CASE("Vec2 normalize")
{
    Vec2D v = Vec2D{3, 4}.normalize();
    REQUIRE(v.x == Approx(0.6f));
    REQUIRE(v.y == Approx(0.8f));
    REQUIRE(v.length() == Approx(1.0f));
}

TEST_CASE("Vec2 normalize zero vector")
{
    Vec2D v = Vec2D{0, 0}.normalize();
    REQUIRE(!std::isnan(v.x));
    REQUIRE(!std::isnan(v.y));
}

TEST_CASE("Vec2 addition")
{
    Vec2D a{1, 2};
    Vec2D b{3, 4};
    Vec2D c = a + b;
    REQUIRE(c.x == Approx(4.0f));
    REQUIRE(c.y == Approx(6.0f));
}

TEST_CASE("Vec2 length")
{
    REQUIRE(Vec2D{3, 4}.length() == Approx(5.0f));
    REQUIRE(Vec2D{0, 0}.length() == Approx(0.0f));
}

TEST_CASE("Vec2 lerp")
{
    Vec2D a{0, 0};
    Vec2D b{10, 10};
    Vec2D mid = a.lerp(b, 0.5f);
    REQUIRE(mid.x == Approx(5.0f));
    REQUIRE(mid.y == Approx(5.0f));
}

TEST_CASE("Vec3 cross product — basic")
{
    Vec3D x{1, 0, 0};
    Vec3D y{0, 1, 0};
    Vec3D z = x.cross(y);
    REQUIRE(z.x == Approx(0.0f));
    REQUIRE(z.y == Approx(0.0f));
    REQUIRE(z.z == Approx(1.0f));
}

TEST_CASE("Vec3 cross product — anti-commutative")
{
    Vec3D a{1, 2, 3};
    Vec3D b{4, 5, 6};
    Vec3D ab = a.cross(b);
    Vec3D ba = b.cross(a);
    REQUIRE(ab.x == Approx(-ba.x));
    REQUIRE(ab.y == Approx(-ba.y));
    REQUIRE(ab.z == Approx(-ba.z));
}

TEST_CASE("Vec3 cross product — parallel gives zero")
{
    Vec3D a{1, 0, 0};
    Vec3D b{2, 0, 0};
    Vec3D result = a.cross(b);
    REQUIRE(result.length() == Approx(0.0f));
}

TEST_CASE("Vec3 reflect")
{
    Vec3D v{1, -1, 0};
    Vec3D n{0, 1, 0};
    Vec3D r = v.reflect(n);
    REQUIRE(r.x == Approx(1.0f));
    REQUIRE(r.y == Approx(1.0f));
    REQUIRE(r.z == Approx(0.0f));
}

TEST_CASE("Vec4 perspective divide")
{
    Vec4D v{2, 4, 6, 2};
    Vec3D result = v.perspectiveDivide();
    REQUIRE(result.x == Approx(1.0f));
    REQUIRE(result.y == Approx(2.0f));
    REQUIRE(result.z == Approx(3.0f));
}

TEST_CASE("Vec4 toVec3D drops w")
{
    Vec4D v{1, 2, 3, 99};
    Vec3D result = v.toVec3D();
    REQUIRE(result.x == Approx(1.0f));
    REQUIRE(result.y == Approx(2.0f));
    REQUIRE(result.z == Approx(3.0f));
}

TEST_CASE("Vec4 dot product")
{
    Vec4D a{1, 0, 0, 0};
    Vec4D b{0, 1, 0, 0};
    REQUIRE(a.dot(b) == Approx(0.0f));

    Vec4D c{1, 2, 3, 4};
    Vec4D d{1, 2, 3, 4};
    REQUIRE(c.dot(d) == Approx(30.0f));
}

TEST_CASE("Vec4 subtraction")
{
    Vec4D a{4, 6, 8, 10};
    Vec4D b{1, 2, 3, 4};
    Vec4D result = a - b;
    REQUIRE(result.x == Approx(3.0f));
    REQUIRE(result.y == Approx(4.0f));
    REQUIRE(result.z == Approx(5.0f));
    REQUIRE(result.w == Approx(6.0f));
}