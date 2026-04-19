#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../include/math/mat.h"
#include "../include/math/vec.h"
using namespace nitro::math;
using Catch::Approx;
TEST_CASE("Mat4 identity — transforms point unchanged")
{
    Mat4 I = Mat4::identity();
    Vec4D p{1, 2, 3, 1};
    Vec4D result = I * p;
    REQUIRE(result.x == Approx(1.0f));
    REQUIRE(result.y == Approx(2.0f));
    REQUIRE(result.z == Approx(3.0f));
}

TEST_CASE("Mat4 translation — moves point correctly")
{
    Mat4 T = Mat4::translation({10, 20, 30});
    Vec4D p{1, 2, 3, 1}; // w=1: translation applies
    Vec4D result = T * p;
    REQUIRE(result.x == Approx(11.0f));
    REQUIRE(result.y == Approx(22.0f));
    REQUIRE(result.z == Approx(33.0f));
}

TEST_CASE("Mat4 translation — does NOT move direction")
{
    Mat4 T = Mat4::translation({10, 20, 30});
    Vec4D d{1, 0, 0, 0}; // w=0: translation ignored
    Vec4D result = T * d;
    REQUIRE(result.x == Approx(1.0f)); // unchanged
    REQUIRE(result.y == Approx(0.0f));
}

TEST_CASE("Mat4 inverse — M * M^-1 = identity")
{
    Mat4 M = Mat4::translation({3, 5, 7}) * Mat4::scale({2, 2, 2});
    Mat4 inv = M.inverse();
    Mat4 shouldBeIdentity = M * inv;
    // Each diagonal element should be 1, off-diagonal near 0
    REQUIRE(shouldBeIdentity[0] == Approx(1.0f).margin(1e-5f));
    REQUIRE(shouldBeIdentity[5] == Approx(1.0f).margin(1e-5f));
    REQUIRE(shouldBeIdentity[10] == Approx(1.0f).margin(1e-5f));
    REQUIRE(shouldBeIdentity[15] == Approx(1.0f).margin(1e-5f));
    REQUIRE(shouldBeIdentity[4] == Approx(0.0f).margin(1e-5f));
}

TEST_CASE("Mat4 scale — scales point correctly")
{
    Mat4 S = Mat4::scale({2, 3, 4});
    Vec4D p{1, 2, 3, 1};

    Vec4D result = S * p;

    REQUIRE(result.x == Approx(2.0f));
    REQUIRE(result.y == Approx(6.0f));
    REQUIRE(result.z == Approx(12.0f));
}

TEST_CASE("Mat4 rotationX — rotates correctly")
{
    Mat4 R = Mat4::rotationX(M_PI / 2.0f);
    Vec4D p{0, 1, 0, 1};

    Vec4D result = R * p;

    REQUIRE(result.x == Approx(0.0f).margin(1e-5));
    REQUIRE(result.y == Approx(0.0f).margin(1e-5));
    REQUIRE(result.z == Approx(1.0f).margin(1e-5));
}

TEST_CASE("Mat4 rotationY — rotates correctly")
{
    Mat4 R = Mat4::rotationY(M_PI / 2.0f);
    Vec4D p{1, 0, 0, 1};

    Vec4D result = R * p;

    REQUIRE(result.x == Approx(0.0f).margin(1e-5));
    REQUIRE(result.z == Approx(-1.0f).margin(1e-5));
}

TEST_CASE("Mat4 rotationZ — rotates correctly")
{
    Mat4 R = Mat4::rotationZ(M_PI / 2.0f);
    Vec4D p{1, 0, 0, 1};

    Vec4D result = R * p;

    REQUIRE(result.x == Approx(0.0f).margin(1e-5));
    REQUIRE(result.y == Approx(1.0f).margin(1e-5));
}

TEST_CASE("Mat4 composition — order matters")
{
    Mat4 T = Mat4::translation({10, 0, 0});
    Mat4 S = Mat4::scale({2, 2, 2});

    Vec4D p{1, 0, 0, 1};

    Vec4D a = (T * S) * p;
    Vec4D b = (S * T) * p;

    REQUIRE(a.x != Approx(b.x));
}

TEST_CASE("Mat4 inverse — translation")
{
    Mat4 T = Mat4::translation({5, -3, 2});
    Mat4 inv = T.inverse();

    Vec4D p{1, 2, 3, 1};

    Vec4D result = inv * (T * p);

    REQUIRE(result.x == Approx(1.0f));
    REQUIRE(result.y == Approx(2.0f));
    REQUIRE(result.z == Approx(3.0f));
}
TEST_CASE("Mat4 inverse — scale")
{
    Mat4 S = Mat4::scale({2, 4, 8});
    Mat4 inv = S.inverse();

    Vec4D p{1, 2, 3, 1};

    Vec4D result = inv * (S * p);

    REQUIRE(result.x == Approx(1.0f));
    REQUIRE(result.y == Approx(2.0f));
    REQUIRE(result.z == Approx(3.0f));
}

TEST_CASE("Mat4 rotation — inverse equals transpose")
{
    Mat4 R = Mat4::rotationZ(0.7f);

    Mat4 inv = R.inverse();
    Mat4 trans = R.transpose();

    for (int i = 0; i < 16; i++)
    {
        REQUIRE(inv.m[i] == Approx(trans.m[i]).margin(1e-5));
    }
}
TEST_CASE("Mat4 determinant — identity is 1")
{
    Mat4 I = Mat4::identity();
    REQUIRE(I.determinant() == Approx(1.0f));
}

TEST_CASE("Mat4 determinant — scale multiplies determinant")
{
    Mat4 S = Mat4::scale({2, 3, 4});
    REQUIRE(S.determinant() == Approx(24.0f));
}

TEST_CASE("Mat4 perspective — w component encodes depth")
{
    Mat4 proj = Mat4::perspective(M_PI / 4.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    Vec4D p = proj * Vec4D{0, 0, -5, 1};
    REQUIRE(p.w != Approx(0.0f));
}

TEST_CASE("Mat4 lookAt — eye at origin looks down -Z")
{
    Mat4 view = Mat4::lookAt({0, 0, 5}, {0, 0, 0}, {0, 1, 0});

    Vec4D p = view * Vec4D{0, 0, 0, 1};
    REQUIRE(p.z < 0.0f);
}

TEST_CASE("perspective — point at near plane maps to z = -1 in NDC")
{
    Mat4 P = Mat4::perspective(M_PI / 4.0f, 1.0f, 0.1f, 100.0f);
    Vec4D nearPoint{0, 0, -0.1f, 1};
    Vec4D clip = P * nearPoint;
    float ndcZ = clip.z / clip.w;
    REQUIRE(ndcZ == Approx(-1.0f).margin(1e-4f));
}

TEST_CASE("lookAt — forward direction is correct")
{
    Mat4 V = Mat4::lookAt({0, 0, 5}, {0, 0, 0}, {0, 1, 0});
    // Origin in world should map to (0,0,-5) in view space
    Vec4D origin{0, 0, 0, 1};
    Vec4D inView = V * origin;
    REQUIRE(inView.z == Approx(-5.0f).margin(1e-5f));
}

TEST_CASE("TRS order — scale then rotate then translate")
{
    Mat4 T = Mat4::translation({10, 0, 0});
    Mat4 R = Mat4::rotationY(M_PI / 2.0f);
    Mat4 S = Mat4::scale({2, 2, 2});
    Mat4 TRS = T * R * S; // applied right to left: scale, rotate, translate
    Vec4D p{1, 0, 0, 1};
    Vec4D result = TRS * p;
    // Scale: (2,0,0) -> Rotate Y 90deg: (0,0,-2) -> Translate: (10,0,-2)
    REQUIRE(result.x == Approx(10.0f).margin(1e-4f));
    REQUIRE(result.z == Approx(-2.0f).margin(1e-4f));
}

TEST_CASE("orthographic — point maps correctly")
{
    Mat4 ortho = Mat4::orthographic(-1, 1, -1, 1, -1, 1);
    Vec4D p = ortho * Vec4D{0, 0, 0, 1};
    REQUIRE(p.x == Approx(0.0f));
    REQUIRE(p.y == Approx(0.0f));
}