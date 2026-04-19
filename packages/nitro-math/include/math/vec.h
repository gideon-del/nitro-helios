#pragma once
#include <iostream>
#include <algorithm>
namespace nitro::math
{
    struct Vec2D
    {
        float x, y;

        Vec2D() : x(0.0f), y(0.0f) {}
        Vec2D(float x, float y) : x(x), y(y) {}

        Vec2D operator+(const Vec2D &o) const
        {
            return {x + o.x, y + o.y};
        }
        Vec2D operator-(const Vec2D &o) const
        {
            return {x - o.x, y - o.y};
        }
        Vec2D operator*(const float t) const
        {
            return {x * t, y * t};
        }
        Vec2D operator/(const float t) const
        {
            return {x / t, y / t};
        }
        float dot(const Vec2D &o) const
        {
            return (x * o.x) + (y * o.y);
        }
        float cross(const Vec2D &o) const
        {
            return (x * o.y) - (y * o.x);
        }
        float lengthSquared() const
        {
            return (x * x) + (y * y);
        }
        float length() const
        {
            return std::sqrt(lengthSquared());
        }
        Vec2D normalize() const
        {
            float len = length();

            if (len < 1e-6)
            {
                return {1, 0};
            }

            return {x / len, y / len};
        }

        Vec2D lerp(const Vec2D &o, const float t) const
        {

            return *this + ((o - *this) * t);
        }

        Vec2D perpendicular()
        {
            return {-y, x};
        }
        float angleBetween(const Vec2D &o) const
        {
            float dotProd = normalize().dot(o.normalize());
            // Clamp dot product for numeric stability
            return std::acos(std::min(1.0f, std::max(dotProd, -1.0f)));
        }

        float signedAngleBetween(const Vec2D &o) const
        {
            return std::atan2(cross(o), dot(o));
        }
    };

    struct Vec3D
    {
        float x, y, z;
        Vec3D() : x(0), y(0), z(0) {}
        Vec3D(float x, float y, float z) : x(x), y(y), z(z) {}

        Vec3D operator+(const Vec3D &o) const
        {
            return {x + o.x, y + o.y, z + o.z};
        }
        Vec3D operator-(const Vec3D &o) const
        {
            return {x - o.x, y - o.y, z - o.z};
        }
        Vec3D operator-() const
        {
            return {-x, -y, -z};
        }
        Vec3D operator*(const float t) const
        {
            return {x * t, y * t, z * t};
        }
        Vec3D operator/(const float t) const
        {
            return {x / t, y / t, z / t};
        }
        float dot(const Vec3D &o) const
        {
            return (x * o.x) + (y * o.y) + (z * o.z);
        }

        float lengthSquared() const
        {
            return (x * x) + (y * y) + (z * z);
        }
        float length() const
        {
            return std::sqrt(lengthSquared());
        }
        Vec3D normalize() const
        {
            float len = length();

            if (len < 1e-6)
            {
                return {1, 0, 0};
            }

            return {x / len, y / len, z / len};
        }
        float angleBetween(const Vec3D &o) const
        {
            float dotProd = normalize().dot(o.normalize());
            // Clamp dot product for numeric stability
            return std::acos(std::min(1.0f, std::max(dotProd, -1.0f)));
        }

        float signedAngleBetween(const Vec3D &o) const
        {

            return std::atan2(cross(o).length(), dot(o));
        }

        Vec3D lerp(const Vec3D &o, const float t) const
        {

            return *this + ((o - *this) * t);
        }
        Vec3D cross(const Vec3D &o) const
        {
            return {
                y * o.z - z * o.y,
                z * o.x - x * o.z,
                x * o.y - y * o.x};
        }

        Vec3D reflect(const Vec3D &o)
        {
            return *this - o * 2.0f * dot(o);
        }

        Vec3D project(const Vec3D &o)
        {
            return o * (dot(o) / o.dot(o));
        }

        Vec2D toVec2D()
        {
            return {x, y};
        }
    };

    struct Vec4D
    {
        float x, y, z, w;

        Vec4D() : x(0), y(0), z(0), w(0) {}
        Vec4D(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
        Vec4D operator+(const Vec4D &o) const
        {
            return {x + o.x, y + o.y, z + o.z, w + o.w};
        }
        Vec4D operator-(const Vec4D &o) const
        {
            return {x - o.x, y - o.y, z - o.z, w - o.w};
        }
        Vec4D operator-() const
        {
            return {-x, -y, -z, -w};
        }
        Vec4D operator*(const float t) const
        {
            return {x * t, y * t, z * t, w * t};
        }
        Vec4D operator/(const float t) const
        {
            return {x / t, y / t, z / t, w / t};
        }

        float dot(const Vec4D &o) const
        {
            return (x * o.x) + (y * o.y) + (z * o.z) + (w * o.w);
        }

        float lengthSquared() const
        {
            return (x * x) + (y * y) + (z * z) + (w * w);
        }
        float length() const
        {
            return std::sqrt(lengthSquared());
        }
        Vec4D normalize() const
        {
            float len = length();

            if (len < 1e-6)
            {
                return {1, 0, 0, 0};
            }

            return {x / len, y / len, z / len, w / len};
        }

        Vec3D perspectiveDivide()
        {
            return {x / w, y / w, z / w};
        }
        Vec3D toVec3D()
        {
            return {x, y, z};
        }
    };

};