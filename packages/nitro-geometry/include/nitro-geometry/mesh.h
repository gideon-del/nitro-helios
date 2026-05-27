#pragma once
#include "vertex.h"
#include <cstdint>

namespace nitro::geometry
{
    struct Mesh
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        void calculateNormals()
        {

            for (uint32_t tri = 0; tri < indices.size(); tri += 3)
            {
                uint32_t i0 = indices[tri];
                uint32_t i1 = indices[tri + 1];
                uint32_t i2 = indices[tri + 2];
                glm::vec3 A = vertices[i0].pos;
                glm::vec3 B = vertices[i1].pos;
                glm::vec3 C = vertices[i2].pos;

                glm::vec3 normal = glm::normalize(glm::cross(B - A, C - A));

                vertices[i0].normal += normal;
                vertices[i1].normal += normal;
                vertices[i2].normal += normal;
            }

            for (auto &vertex : vertices)
            {
                vertex.normal = glm::normalize(vertex.normal);
            }
        };
    };
} // namespace nitro::rhi
