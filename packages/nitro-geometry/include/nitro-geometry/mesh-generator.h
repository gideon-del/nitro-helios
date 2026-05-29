#pragma once
#include "mesh.h"
#include <vector>
namespace nitro::geometry
{
    constexpr float EPSILON = 1e-6f;
    class MeshGenerator
    {
    public:
        static Mesh createQuad(float width, float height)
        {
            float halfWidth = width * 0.5f;
            float halfHeight = height * 0.5f;

            float leftX = 0.0f - halfWidth;
            float rightX = 0.0f + halfWidth;

            float topY = 0.0f + halfHeight;
            float bottomY = 0.0f - halfHeight;

            Vertex v0({leftX, topY, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f});
            Vertex v1({rightX, topY, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f});
            Vertex v2({rightX, bottomY, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f});
            Vertex v3{{leftX, bottomY, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}};

            return Mesh{
                .vertices = {v0, v1, v2, v3},
                .indices = {0, 1, 2, 2, 3, 0}};
        }

        static Mesh createPlane(float width, float depth)
        {

            float halfWidth = width * 0.5f;
            float halfDepth = depth * 0.5f;

            float leftX = 0.0f - halfWidth;
            float rightX = 0.0f + halfWidth;

            float farZ = 0.0f + halfDepth;
            float nearZ = 0.0f - halfDepth;

            Vertex v0{{leftX, 0.0f, farZ}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}};
            Vertex v1{{rightX, 0.0f, farZ}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}};
            Vertex v2{{rightX, 0.0f, nearZ}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}};
            Vertex v3{{leftX, 0.0f, nearZ}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}};

            return Mesh{
                .vertices = {v0, v1, v2, v3},
                .indices = {0, 1, 2, 2, 3, 0}};
        }

        static Mesh createGrid(uint32_t rows, uint32_t cols, float width, float height)
        {

            float widthPerCell = width / float(cols);
            float heightPerCell = height / float(rows);

            float halfWidth = width * 0.5f;
            float halfHeight = height * 0.5f;

            glm::vec3 leftTopPos{0.0f - halfWidth, 0.0f + halfHeight, 0.0f};

            Mesh mesh;

            for (int row = 0; row <= rows; row++)
            {
                float y = (float)row * heightPerCell;

                for (int col = 0; col <= cols; col++)
                {
                    float x = (float)col * widthPerCell;
                    uint32_t idx = col;
                    float r = (idx + 0) % 3;
                    float g = (idx + 1) % 3;
                    float b = (idx + 2) % 3;
                    mesh.vertices.push_back({leftTopPos + glm::vec3{x, -y, 0.0f},
                                             {r, g, b},
                                             {float(col) / float(cols), float(row) / float(rows)}});
                }
            }

            for (int col = 0; col < cols; col++)
            {
                for (int row = 0; row < rows; row++)
                {
                    uint32_t topLeft = (row * (cols + 1)) + col;
                    uint32_t topRight = topLeft + 1;
                    uint32_t bottomLeft = topLeft + (cols + 1);
                    uint32_t bottomRight = bottomLeft + 1;
                    mesh.indices.push_back(topLeft);
                    mesh.indices.push_back(topRight);
                    mesh.indices.push_back(bottomRight);

                    mesh.indices.push_back(bottomRight);
                    mesh.indices.push_back(bottomLeft);
                    mesh.indices.push_back(topLeft);
                }
            }

            return mesh;
        }
        static Mesh createPolygon(uint32_t radius, uint32_t segments)
        {
            float sideAngle = (M_PI * 2) / float(segments);
            Mesh mesh;

            glm::vec3 staringPoint{0.0f, 0.0f, 0.0f};

            mesh.vertices.push_back({glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 1.0f)});
            for (int i = 0; i < segments; i++)
            {

                float currentAngle = sideAngle * i;
                float x = std::cos(currentAngle);
                if (std::abs(x) < EPSILON)
                {
                    x = 0;
                }
                float y = std::sin(currentAngle);
                if (std::abs(y) < EPSILON)
                {
                    y = 0;
                }

                x *= radius;
                y *= radius;
                float u = ((x) / radius + 1) / 2;
                float v = ((y) / radius + 1) / 2;
                mesh.vertices.push_back({staringPoint + glm::vec3{x, y, 0.0f},
                                         {1.0f, 0.0f, 0.0f},
                                         {u, v}});
            }

            for (int i = 1; i <= segments; i++)
            {

                int nextIdx = i == segments ? 1 : i + 1;
                mesh.indices.push_back(0);
                mesh.indices.push_back(nextIdx);
                mesh.indices.push_back(i);
            }

            return mesh;
        };

        static Mesh createCircle(uint32_t radius)
        {
            float sideAngle = M_PI * 10e-3;

            uint32_t segments = static_cast<uint32_t>((M_PI * 2) / sideAngle);

            float EPSILON = 1e-6f;
            Mesh mesh;

            glm::vec3 staringPoint{0.0f, 0.0f, 0.0f};

            mesh.vertices.push_back({glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 1.0f)});
            for (int i = 0; i < segments; i++)
            {

                float currentAngle = sideAngle * i;
                float x = std::cos(currentAngle);
                if (std::abs(x) < EPSILON)
                {
                    x = 0;
                }
                float y = std::sin(currentAngle);
                if (std::abs(y) < EPSILON)
                {
                    y = 0;
                }
                x *= radius;
                y *= radius;
                float u = ((x) / radius + 1) / 2;
                float v = ((y) / radius + 1) / 2;

                mesh.vertices.push_back({staringPoint + glm::vec3{x, y, 0.0f},
                                         {1.0f, 0.0f, 0.0f},
                                         {u, v}});
            }

            for (int i = 1; i <= segments; i++)
            {

                int nextIdx = i == segments ? 1 : i + 1;
                mesh.indices.push_back(0);
                mesh.indices.push_back(nextIdx);
                mesh.indices.push_back(i);
            }

            return mesh;
        }

        static Mesh createRing(
            float innerRadius,
            float outerRadius,
            uint32_t segments)
        {
            float sideAngle = (M_PI * 2) / float(segments);
            float uvIncrease = 1.0f / float(segments + 1);
            Mesh mesh;

            for (int i = 0; i <= segments; i++)
            {
                float currentAngle = sideAngle * i;
                float x = std::cos(currentAngle);
                if (std::abs(x) < EPSILON)
                {
                    x = 0;
                }
                float y = std::sin(currentAngle);
                if (std::abs(y) < EPSILON)
                {
                    y = 0;
                }
                float finalRadius = outerRadius + innerRadius;
                Vertex Iv{
                    glm::vec3(innerRadius * x, innerRadius * y, 0.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    {1, uvIncrease * i}};

                Vertex Ov{
                    glm::vec3(finalRadius * x, finalRadius * y, 0.0f),
                    glm::vec3(1.0f, 0.0f, 1.0f),
                    {0, uvIncrease * i}};

                mesh.vertices.push_back(Ov);
                mesh.vertices.push_back(Iv);
            }

            for (int i = 0; i < segments; i++)
            {
                uint32_t topRight = 2 * i;
                uint32_t bottomRight = topRight + 1;

                uint32_t topLeft = bottomRight + 1;
                uint32_t bottomLeft = topLeft + 1;
                // if (i + 1 == segments)
                // {
                //     topLeft = 0;
                //     bottomLeft = 1;
                // }

                mesh.indices.push_back(topLeft);
                mesh.indices.push_back(topRight);
                mesh.indices.push_back(bottomRight);

                mesh.indices.push_back(bottomRight);
                mesh.indices.push_back(bottomLeft);
                mesh.indices.push_back(topLeft);
            }

            return mesh;
        }
        static Mesh createCylinder(
            float radius,
            float height,
            uint32_t segments)
        {
            float sideAngle = (M_PI * 2) / float(segments);
            float uIncrease = 1.0f / float(segments + 1);
            float halfHeight = height * 0.5f;
            Mesh mesh;
            for (int i = 0; i <= segments; i++)
            {
                float currentAngle = sideAngle * i;
                float x = std::sin(currentAngle);
                if (std::abs(x) < EPSILON)
                {
                    x = 0;
                }
                float z = std::cos(currentAngle);
                if (std::abs(z) < EPSILON)
                {
                    z = 0;
                }
                float r = (i + 0) % 3;
                float g = (i + 1) % 3;
                float b = (i + 2) % 3;

                Vertex Iv{
                    glm::vec3(radius * x, -halfHeight, radius * z),
                    glm::vec3(r, g, b),
                    {uIncrease * i, 1.0f}};

                Vertex Ov{
                    glm::vec3(radius * x, halfHeight, radius * z),
                    glm::vec3(0.5f, 0.5f, 0.5f),
                    {uIncrease * i, 0.0f}};

                mesh.vertices.push_back(Ov);
                mesh.vertices.push_back(Iv);
            }

            for (int i = 0; i < segments; i++)
            {
                uint32_t topRight = 2 * i;
                uint32_t bottomRight = topRight + 1;

                uint32_t topLeft = bottomRight + 1;
                uint32_t bottomLeft = topLeft + 1;
                // if (i + 1 == segments)
                // {
                //     topLeft = 0;
                //     bottomLeft = 1;
                // }

                mesh.indices.push_back(topLeft);
                mesh.indices.push_back(topRight);
                mesh.indices.push_back(bottomRight);

                mesh.indices.push_back(topLeft);
                mesh.indices.push_back(bottomRight);
                mesh.indices.push_back(bottomLeft);
            }

            return mesh;
        }

        static Mesh createUVSphere(uint32_t radius, uint32_t rings, uint32_t segments)
        {

            float phiAngle = (M_PI * 2) / float(segments);
            float thetaAngle = M_PI / float(rings);
            glm::vec3 startPos{0.0f, 0.0f, 1.0f};
            Mesh mesh;

            for (int ring = 0; ring <= rings; ring++)
            {
                float currentThetaAngle = thetaAngle * float(ring);
                float y = std::cos(currentThetaAngle);
                if (std::abs(y) < EPSILON)
                {
                    y = 0;
                }

                float ringRadius = std::sin(currentThetaAngle);
                if (std::abs(ringRadius) < EPSILON)
                {
                    ringRadius = 0;
                }
                ringRadius *= radius;

                for (int i = 0; i <= segments; i++)
                {
                    float currentAngle = phiAngle * i;
                    float x = std::cos(currentAngle);
                    if (std::abs(x) < EPSILON)
                    {
                        x = 0.0f;
                    }
                    float z = std::sin(currentAngle);
                    if (std::abs(z) < EPSILON)
                    {
                        z = 0.0f;
                    }

                    float r = (ring + 0) % 3;
                    float g = (ring + 1) % 3;
                    float b = (ring + 2) % 3;

                    float u = currentAngle / (2 * M_PI);
                    float v = currentThetaAngle / (M_PI);
                    mesh.vertices.push_back({startPos + glm::vec3(ringRadius * x, radius * y, ringRadius * z),
                                             glm::vec3(r, g, b),
                                             {u, v}});
                }
            }

            for (int ring = 0; ring < rings; ring++)
            {
                for (int segment = 0; segment < segments; segment++)
                {
                    uint32_t topLeft = (ring * (segments + 1)) + segment;
                    uint32_t bottomLeft = topLeft + (segments + 1);
                    uint32_t topRight = topLeft + 1;
                    uint32_t bottomRight = bottomLeft + 1;

                    mesh.indices.push_back(topLeft);
                    mesh.indices.push_back(topRight);
                    mesh.indices.push_back(bottomRight);

                    mesh.indices.push_back(bottomRight);
                    mesh.indices.push_back(bottomLeft);
                    mesh.indices.push_back(topLeft);
                }
            }
            return mesh;
        };

        static Mesh createCone(
            float radius,
            float height,
            uint32_t segments)
        {
            float sideAngles = (M_PI) * 2 / float(segments);

            float halfHeight = float(height) * 0.5f;
            float uIncrease = 1.0f / float(segments + 1);
            Mesh mesh;

            for (int i = 0; i <= segments; i++)
            {
                float currentAngle = sideAngles * i;
                float x = std::sin(currentAngle);
                if (std::abs(x) < EPSILON)
                {
                    x = 0;
                }
                float z = std::cos(currentAngle);
                if (std::abs(z) < EPSILON)
                {
                    z = 0;
                }
                float r = (i + 0) % 3;
                float g = (i + 1) % 3;
                float b = (i + 2) % 3;
                mesh.vertices.push_back(Vertex{
                    {0.0f, halfHeight, 0.0f},
                    {0.0f, 0.0f, 0.0f},
                    {uIncrease * i, 0.0f}});
                mesh.vertices.push_back({glm::vec3{radius * x, -halfHeight, radius * z},
                                         glm::vec3(r, g, b),
                                         {uIncrease * i, 1.0f}});
            }

            for (int i = 0; i < segments; i++)
            {

                uint32_t A = 2 * i;
                uint32_t C = A + 1;
                uint32_t B = 2 + C;

                                // if (i == segments)
                // {
                //     B = 1;
                // }

                mesh.indices.push_back(A);
                mesh.indices.push_back(B);
                mesh.indices.push_back(C);
            }

            return mesh;
        }

        static Mesh createNormalVisualization(const Mesh &mesh, float length)
        {
            Mesh normalMesh;

            for (const auto &vertex : mesh.vertices)
            {
                Vertex start{
                    (vertex.pos),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    vertex.normal};
                Vertex end{
                    vertex.pos + vertex.normal * length,
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    vertex.normal};

                normalMesh.vertices.push_back(start);
                normalMesh.vertices.push_back(end);
            }

            for (int i = 0; i < mesh.vertices.size(); i++)
            {
                uint32_t currentP = 2 * i;
                uint32_t endP = currentP + 1;
                normalMesh.indices.push_back(currentP);

                normalMesh.indices.push_back(endP);
            }

            return normalMesh;
        }
    };

} // namespace nitro::geometry
