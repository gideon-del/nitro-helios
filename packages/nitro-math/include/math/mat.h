#pragma once
#include <iostream>
#include "vec.h"
namespace nitro::math
{

    struct Mat3
    {
        float m[9];

        Mat3()
        {
            for (int i = 0; i < 9; i++)
                m[i] = 0.0f;
            m[0] = m[4] = m[8] = 1;
        }

        float det() const
        {
            float a = m[0] * (m[4] * m[8] - m[5] * m[7]);
            float b = m[3] * (m[1] * m[8] - m[7] * m[2]);
            float c = m[6] * (m[1] * m[5] - m[4] * m[2]);
            return a - b + c;
        }
        float &at(int col, int row) { return m[col * 3 + row]; }
        float at(int col, int row) const { return m[col * 3 + row]; }
    };

    struct Mat4
    {
        float m[16];
        Mat4()
        {
            for (int i = 0; i < 16; i++)
                m[i] = 0.0f;
            m[0] = m[5] = m[10] = m[15] = 1.0f;
        }

        float &at(int col, int row) { return m[col * 4 + row]; }
        float at(int col, int row) const { return m[col * 4 + row]; }

        Mat4 operator*(const Mat4 &o) const
        {
            Mat4 result;
            result.m[0] = 0;
            result.m[5] = 0;
            result.m[10] = 0;
            result.m[15] = 0;

            for (int col = 0; col < 4; col++)
            {
                for (int row = 0; row < 4; row++)
                {
                    float sum = 0;
                    for (int k = 0; k < 4; k++)
                        sum += at(k, row) * o.at(col, k);
                    result.at(col, row) = sum;
                }
            }

            return result;
        }

        Vec4D operator*(const Vec4D &v)
        {
            return {
                m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w,
                m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
                m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w,
                m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w,
            };
        }
        float operator[](const int &idx) { return m[idx]; }

        Mat4 transpose() const
        {
            Mat4 result;

            for (int row = 0; row < 4; row++)
            {
                for (int col = 0; col < 4; col++)
                {
                    result.at(row, col) = at(col, row);
                }
            }

            return result;
        }

        Mat3 extractMinor(int col, int row) const
        {
            Mat3 result;
            for (int i = 0; i < 9; i++)
                result.m[i] = 0.0f;
            int subCol = 0;

            for (int c = 0; c < 4; c++)
            {
                if (c == col)
                    continue;

                int subRow = 0;

                for (int r = 0; r < 4; r++)
                {
                    if (r == row)
                        continue;

                    result.at(subCol, subRow) = at(c, r);
                    subRow++;
                }

                subCol++;
            }

            return result;
        }

        float cofactor(int col, int row) const
        {
            float minorDet = extractMinor(col, row).det();
            return ((col + row) % 2 == 0) ? minorDet : -minorDet;
        }
        Mat4 cofactorMatrix() const
        {
            Mat4 result;

            for (int col = 0; col < 4; col++)
            {
                for (int row = 0; row < 4; row++)
                {
                    result.at(col, row) = cofactor(col, row);
                }
            }

            return result;
        }

        Mat4 adjugate() const
        {
            return cofactorMatrix().transpose();
        }
        float determinant() const
        {
            float a11 = m[0];
            float a12 = m[4];
            float a13 = m[8];
            float a14 = m[12];

            float C11 = cofactor(0, 0);
            float C12 = cofactor(1, 0);
            float C13 = cofactor(2, 0);
            float C14 = cofactor(3, 0);

            return (a11 * C11) + (a12 * C12) + (a13 * C13) + (a14 * C14);
        }

        Mat4 inverse() const
        {
            float det = determinant();
            if (det == 0.0f)
            {
                return Mat4::identity();
            }
            Mat4 adj = adjugate();
            Mat4 result;

            for (int col = 0; col < 4; col++)
            {
                for (int row = 0; row < 4; row++)
                {
                    result.at(col, row) = adj.at(col, row) / det;
                }
            }

            return result;
        }

        static Mat4 identity() { return Mat4(); }
        static Mat4 translation(const Vec3D &v)
        {
            Mat4 m;
            m.at(3, 0) = v.x;
            m.at(3, 1) = v.y;
            m.at(3, 2) = v.z;
            return m;
        }
        static Mat4 scale(const Vec3D &v)
        {
            Mat4 m;
            m.at(0, 0) = v.x;
            m.at(1, 1) = v.y;
            m.at(2, 2) = v.z;
            return m;
        }

        static Mat4 rotationX(float angle)
        {
            Mat4 m;
            m.at(1, 1) = std::cos(angle);
            m.at(1, 2) = std::sin(angle);
            m.at(2, 1) = -std::sin(angle);
            m.at(2, 2) = std::cos(angle);

            return m;
        }
        static Mat4 rotationY(float angle)
        {
            Mat4 m;
            m.at(0, 0) = std::cos(angle);
            m.at(0, 2) = -std::sin(angle);
            m.at(2, 0) = std::sin(angle);
            m.at(2, 2) = std::cos(angle);

            return m;
        }
        static Mat4 rotationZ(float angle)
        {
            Mat4 m;
            m.at(0, 0) = std::cos(angle);
            m.at(0, 1) = std::sin(angle);
            m.at(1, 0) = -std::sin(angle);
            m.at(1, 1) = std::cos(angle);

            return m;
        }

        static Mat4 perspective(float fovY, float aspect, float near, float far)
        {
            float focalLength = 1.0f / std::tan(fovY / 2.0f);

            Mat4 result;

            for (int i = 0; i < 16; i++)
                result.m[i] = 0.0f;

            result.at(0, 0) = focalLength / aspect;
            result.at(1, 1) = focalLength;
            result.at(2, 2) = (far + near) / (near - far);
            result.at(3, 2) = (2 * near * far) / (near - far);
            result.at(2, 3) = -1.0f;

            return result;
        }
        static Mat4 lookAt(Vec3D eye, Vec3D target, Vec3D up)
        {
            Vec3D f = (target - eye).normalize();
            Vec3D r = f.cross(up).normalize();
            Vec3D u = r.cross(f);

            Mat4 m;

            m.at(0, 0) = r.x;
            m.at(1, 0) = r.y;
            m.at(2, 0) = r.z;

            m.at(0, 1) = u.x;
            m.at(1, 1) = u.y;
            m.at(2, 1) = u.z;

            m.at(0, 2) = -f.x;
            m.at(1, 2) = -f.y;
            m.at(2, 2) = -f.z;

            m.at(3, 0) = -r.dot(eye);
            m.at(3, 1) = -u.dot(eye);
            m.at(3, 2) = f.dot(eye);
            return m;
        }

        static Mat4 normalMatrix(const Mat4 &model)
        {
            return model.inverse().transpose();
        }

        static Mat4 orthographic(float left, float right, float bottom, float top, float near, float far)
        {
            Mat4 m;
            for (int i = 0; i < 16; i++)
                m.m[i] = 0.0f;
            m.at(0, 0) = 2.0f / (right - left);
            m.at(1, 1) = 2.0f / (top - bottom);
            m.at(2, 2) = -2.0f / (far - near);
            m.at(3, 0) = -(right + left) / (right - left);
            m.at(3, 1) = -(top + bottom) / (top - bottom);
            m.at(3, 2) = -(far + near) / (far - near);
            m.at(3, 3) = 1.0f;
            return m;
        }
    };

}