#pragma once
#include <glm/glm.hpp>
#include <nitro-rhi/rhi-pipeline.h>
#include <iostream>
namespace nitro::rhi
{
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;

        static RHIVertexLayout getVertexLayout()
        {
            RHIVertexLayout vertexLayout;

            vertexLayout.binding = 0;
            vertexLayout.stride = sizeof(Vertex);

            vertexLayout.attributes.resize(2);

            vertexLayout.attributes[0].format = RHIVertexLayout::Attributes::Format::Float3;
            vertexLayout.attributes[0].location = 0;
            vertexLayout.attributes[0].offset = offsetof(Vertex, pos);

            vertexLayout.attributes[1].format = RHIVertexLayout::Attributes::Format::Float3;
            vertexLayout.attributes[1].location = 1;
            vertexLayout.attributes[1].offset = offsetof(Vertex, color);

            return vertexLayout;
        }
    };
} // namespace nitro::rhi
