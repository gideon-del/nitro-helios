#pragma once
#include "vec.h"
#include "mat.h"

namespace nitro::math
{
    struct Quat
    {
        float x, y, z, w;

        Quat() : x(0), y(0), z(0), w(1) {};
        Quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {};

        Quat operator*(const Quat &q) const
        {
            return {
                w * q.x + x * q.w + y * q.z - z * q.y,
                w * q.y - x * q.z + y * q.w + z * q.x,
                w * q.z + x * q.y - y * q.x + z * q.w,
                w * q.w - x * q.x - y * q.y - z * q.z};
        }
        float length() const
        {
            return std::sqrt(x * x + y * y + z * z + w * w);
        }
        Quat normalized() const
        {
            float len = length();

            if (len < 1e-6f)
                return {0, 0, 0, 1};
            return {x / len, y / len, z / len, w / len};
        }
        Quat inverse() const
        {
            return {-x, -y, -z, w};
        }

        Vec3D rotatePoint(const Vec3D &v) const
        {
            Quat qv(v.x, v.y, v.z, 0.0f);

            Quat inv = this->inverse();

            Quat result = (*this) * qv * inv;

            return {result.x, result.y, result.z};
        }

        Mat4 toMat4() const
        {
            Quat q = normalized();
            Mat4 m;
            m.at(0, 0) = 1 - 2 * (q.y * q.y + q.z * q.z);
            m.at(1, 0) = 2 * (q.x * q.y - q.z * q.w);
            m.at(2, 0) = 2 * (q.x * q.z + q.y * q.w);

            m.at(0, 1) = 2 * (q.x * q.y + q.z * q.w);
            m.at(1, 1) = 1 - 2 * (q.x * q.x + q.z * q.z);
            m.at(2, 1) = 2 * (q.y * q.z - q.x * q.w);

            m.at(0, 2) = 2 * (q.x * q.z - q.y * q.w);
            m.at(1, 2) = 2 * (q.y * q.z + q.x * q.w);
            m.at(2, 2) = 1 - 2 * (q.x * q.x + q.y * q.y);

            return m;
        }
        static Quat fromAxisAngle(Vec3D axis, float angle)
        {
            if (axis.length() == 0)
                return Quat();
            Vec3D a = axis.normalize();
            float s = std::sin(angle / 2.0f);

            return {a.x * s, a.y * s, a.z * s, std::cos(angle / 2.0f)};
        }
        Quat slerp(const Quat &o, float t)
        {

            float dot = x * o.x + y * o.y + z * o.z + w * o.w;
            Quat end = o;
            if (dot < 0.0f)
            {
                end = {-o.x, -o.y, -o.z, -o.w};
                dot = -dot;
            }
            dot = std::min(dot, 1.0f);

            if (dot > 0.9995f)
            {
                return Quat{
                    x + t * (end.x - x),
                    y + t * (end.y - y),
                    z + t * (end.z - z),
                    w + t * (end.w - w)}
                    .normalized();
            }

            float theta0 = std::acos(dot);
            float theta = theta0 * t;

            float s0 = std::cos(theta) - dot * std::sin(theta) / std::sin(theta0);
            float s1 = std::sin(theta) / std::sin(theta0);

            return {
                s0 * x + s1 * end.x,
                s0 * y + s1 * end.y,
                s0 * z + s1 * end.z,
                s0 * w + s1 * end.w,
            };
        }
    };
}