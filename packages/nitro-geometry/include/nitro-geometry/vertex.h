#pragma once
#include <glm/glm.hpp>
#include <nitro-rhi/rhi-pipeline.h>
#include <iostream>

using namespace nitro::rhi;
namespace nitro::geometry
{
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f);
        glm::vec2 uv;

        Vertex(glm::vec3 pos, glm::vec3 color, glm::vec3 normal, glm::vec2 uv) : pos(pos), color(color), normal(normal), uv(uv)
        {
        }
        Vertex(glm::vec3 pos, glm::vec2 uv) : pos(pos), uv(uv) {}
        Vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 uv) : pos(pos), uv(uv), color(color)
        {
            normal = glm::normalize(pos);
        }

        static RHIVertexLayout getVertexLayout()
        {
            RHIVertexLayout vertexLayout;

            vertexLayout.binding = 0;
            vertexLayout.stride = sizeof(Vertex);

            vertexLayout.attributes.resize(4);

            vertexLayout.attributes[0].format = RHIVertexLayout::Attributes::Format::Float3;
            vertexLayout.attributes[0].location = 0;
            vertexLayout.attributes[0].offset = offsetof(Vertex, pos);

            vertexLayout.attributes[1].format = RHIVertexLayout::Attributes::Format::Float3;
            vertexLayout.attributes[1].location = 1;
            vertexLayout.attributes[1].offset = offsetof(Vertex, color);

            vertexLayout.attributes[2].format = RHIVertexLayout::Attributes::Format::Float3;
            vertexLayout.attributes[2].location = 2;
            vertexLayout.attributes[2].offset = offsetof(Vertex, normal);

            vertexLayout.attributes[3].format = RHIVertexLayout::Attributes::Format::Float2;
            vertexLayout.attributes[3].location = 3;
            vertexLayout.attributes[3].offset = offsetof(Vertex, uv);

            return vertexLayout;
        }
    };
} // namespace nitro::rhi
