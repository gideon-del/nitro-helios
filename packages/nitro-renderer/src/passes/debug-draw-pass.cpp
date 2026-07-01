#include <nitro-renderer/passes/debug-draw-pass.h>
#include <nitro-geometry/mesh-generator.h>
#include <glm/gtc/matrix_transform.hpp>
namespace nitro::renderer
{
    DebugDrawPass::DebugDrawPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal) : m_device(device), m_width(width), m_height(height)
    {
        std::vector<rhi::RHIDescriptorBinding> bindings{
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Vertex,
             2}};
        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::PipelineDesc pipelineDesc;

        pipelineDesc.layouts = {m_descriptorLayout};
        pipelineDesc.topology = rhi::PipelineTopology::LineList;
        pipelineDesc.hasPushConstant = true;
        pipelineDesc.pushConstantSize = sizeof(glm::mat4);
        pipelineDesc.depthWrite = false;
        pipelineDesc.depthTest = rhi::CompareOp::Always;
        std::string shaderPath = shaderDir + "/debug-pass/debug-pass";

        if (isMetal)
        {
            pipelineDesc.shaders.push_back({"vs",
                                            shaderPath + ".metallib",
                                            rhi::ShaderStage::Vertex});
            pipelineDesc.shaders.push_back({"fs",
                                            shaderPath + ".metallib",
                                            rhi::ShaderStage::Fragment});
        }
        else
        {
            pipelineDesc.shaders.push_back({"main",
                                            shaderPath + ".vert.spv",
                                            rhi::ShaderStage::Vertex});
            pipelineDesc.shaders.push_back({"main",
                                            shaderPath + ".frag.spv",
                                            rhi::ShaderStage::Fragment});
        }

        m_pipeline = m_device->createPipeline(pipelineDesc);

        m_resources.create(g_MAX_FRAMES_IN_FLIGHT,
                           [&](uint32_t frameIdx)
                           {
                               DebugPassResource resource;

                               rhi::BufferDesc bufferDesc;
                               bufferDesc.size = sizeof(DebugVertex) * DebugDrawPass::MAX_LINES * 2;
                               bufferDesc.usage = rhi::BufferDesc::Usage::Storage;
                               bufferDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                               resource.vertexStorageBuffer = m_device->createBuffer(bufferDesc);

                               resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

                               resource.descriptorSet->writeBuffer(resource.vertexStorageBuffer, 2);
                               resource.descriptorSet->commit();
                               return resource;
                           });
    }

    DebugDrawPass::~DebugDrawPass()
    {
        for (auto &resource : m_resources)
        {
            m_device->destroyBuffer(resource.vertexStorageBuffer);
            m_device->destroyDescriptorSet(resource.descriptorSet);
        }

        m_device->destroyPipeline(m_pipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
        clear();
    }

    void DebugDrawPass::drawLine(glm::vec3 start, glm::vec3 end, glm::vec3 color)
    {

        if (m_vertices.size() + 2 > DebugDrawPass::MAX_LINES * 2)
        {
            return;
        }

        m_vertices.push_back({glm::vec4(start, 1.0), glm::vec4(color, 1.0)});
        m_vertices.push_back({glm::vec4(end, 1.0), glm::vec4(color, 1.0)});
    }

    void DebugDrawPass::drawRay(glm::vec3 origin, glm::vec3 direction, float length, glm::vec3 color)
    {
        drawLine(origin, origin + glm::normalize(direction) * length, color);
    };

    void DebugDrawPass::drawAxes(const glm::mat4 &transform, float scale)
    {

        glm::vec3 xBasis = glm::vec3(transform[0]);
        glm::vec3 yBasis = glm::vec3(transform[1]);
        glm::vec3 zBasis = glm::vec3(transform[2]);
        glm::vec3 origin = glm::vec3(transform[3]);

        drawRay(origin, xBasis, scale, glm::vec3(1, 0, 0));
        drawRay(origin, yBasis, scale, glm::vec3(0, 1, 0));
        drawRay(origin, zBasis, scale, glm::vec3(0, 0, 1));
    }

    void DebugDrawPass::drawQuad(glm::vec3 c0, glm::vec3 c1, glm::vec3 c2, glm::vec3 c3, glm::vec3 color)
    {
        drawLine(c0, c1, color);
        drawLine(c1, c2, color);
        drawLine(c2, c3, color);
        drawLine(c3, c0, color);
    }
    void DebugDrawPass::drawPlane(
        const glm::vec3 &point,
        const glm::vec3 &normal,
        float size,
        const glm::vec4 color)
    {

        glm::vec3 referenceUp = glm::vec3(0, 1, 0);
        if (glm::abs(glm::dot(normal, referenceUp)) > 0.99)
        {
            referenceUp = glm::vec3(1, 0, 0);
        }

        glm::vec3 inPlaneA = glm::normalize(glm::cross(normal, referenceUp));

        glm::vec3 inPlaneB = glm::normalize(glm::cross(normal, inPlaneA));

        float h = size * 0.5f;

        glm::vec3 c0 = point + inPlaneA * h + inPlaneB * h;
        glm::vec3 c1 = point - inPlaneA * h + inPlaneB * h;
        glm::vec3 c2 = point - inPlaneA * h - inPlaneB * h;
        glm::vec3 c3 = point + inPlaneA * h - inPlaneB * h;

        drawQuad(c0, c1, c2, c3, color);
    };
    void DebugDrawPass::drawAABB(glm::vec3 min, glm::vec3 max, glm::vec3 color)
    {
        glm::vec3 corners[8] = {
            {min.x, min.y, min.z},
            {max.x, min.y, min.z},
            {max.x, max.y, min.z},
            {min.x, max.y, min.z},
            {min.x, min.y, max.z},
            {max.x, min.y, max.z},
            {max.x, max.y, max.z},
            {min.x, max.y, max.z}};
        static const int edges[12][2] = {
            {0, 1},
            {1, 2},
            {2, 3},
            {3, 0}, // Bottom face
            {4, 5},
            {5, 6},
            {6, 7},
            {7, 4}, // top face
            {0, 4},
            {1, 5},
            {2, 6},
            {3, 7}, // vertical connection
        };

        for (auto &edge : edges)
        {
            drawLine(corners[edge[0]], corners[edge[1]], color);
        }
    };

    void DebugDrawPass::drawFrustum(glm::mat4 invViewProj, float near, float far, glm::vec3 color)
    {
        auto convertNDCToWorld = [&](glm::vec3 ndc)
        {
            glm::vec4 clipPos = invViewProj * glm::vec4(ndc, 1.0);

            return glm::vec3(clipPos) / clipPos.w;
        };

        glm::vec3 nearCorners[4] = {
            {convertNDCToWorld({-1, 1, near})},
            {convertNDCToWorld({1, 1, near})},
            {convertNDCToWorld({1, -1, near})},
            {convertNDCToWorld({-1, -1, near})},
        };

        glm::vec3 farCorners[4] = {
            {convertNDCToWorld({-1, 1, far})},
            {convertNDCToWorld({1, 1, far})},
            {convertNDCToWorld({1, -1, far})},
            {convertNDCToWorld({-1, -1, far})},
        };

        drawQuad(farCorners[0], farCorners[1], farCorners[2], farCorners[3], color);
        drawQuad(nearCorners[0], nearCorners[1], nearCorners[2], nearCorners[3], color);

        for (int i = 0; i < 4; i++)
        {
            drawLine(nearCorners[i], farCorners[i], color);
        }
    }

    void DebugDrawPass::drawCircle(
        glm::vec3 center,
        glm::vec3 axisA,
        glm::vec3 axisB,
        float radius,
        glm::vec3 color)
    {

        constexpr int SEGMENTS = 32;

        for (int i = 0; i < SEGMENTS; i++)
        {
            float t0 = glm::two_pi<float>() * float(i) / float(SEGMENTS);
            float t1 = glm::two_pi<float>() * float(i + 1) / float(SEGMENTS);

            glm::vec3 p0 =
                center +
                axisA * cos(t0) * radius +
                axisB * sin(t0) * radius;

            glm::vec3 p1 =
                center +
                axisA * cos(t1) * radius +
                axisB * sin(t1) * radius;

            drawLine(p0, p1, color);
        }
    }
    void DebugDrawPass::drawSphere(glm::vec3 center, float radius, glm::vec3 color)
    {
        drawCircle(
            center,
            glm::vec3(1, 0, 0),
            glm::vec3(0, 1, 0),
            radius,
            color);

        drawCircle(
            center,
            glm::vec3(1, 0, 0),
            glm::vec3(0, 0, 1),
            radius,
            color);

        drawCircle(
            center,
            glm::vec3(0, 1, 0),
            glm::vec3(0, 0, 1),
            radius,
            color);
    };
    void DebugDrawPass::drawTileFrustum(
        glm::vec2 tile,
        glm::vec2 screenSize,
        const glm::mat4 &invProj,
        const glm::mat4 &view,
        float tileNear,
        float tileFar)
    {
        auto reconstructCorner = [&](glm::vec2 px)
        {
            glm::vec2 ndc = (px * glm::vec2(16.0f) / screenSize) * 2.0f - 1.0f;
            glm::vec4 clip = invProj * glm::vec4(ndc, 0.0f, 1.0f);
            return glm::normalize(glm::vec3(clip) / clip.w);
        };
        glm::mat4 invView = glm::inverse(view);
        auto toWorldDir = [&](glm::vec3 d)
        {
            return glm::normalize(
                glm::vec3(invView * glm::vec4(d, 0.0f)));
        };

        glm::vec3 eye = glm::vec3(invView[3]);
        glm::vec3 forward = -glm::vec3(invView[2]);
        glm::vec3 origin = eye;
        // drawSphere(origin, 1.0f, {1, 1, 1});
        glm::vec3 topLeft = glm::normalize(reconstructCorner(tile));
        glm::vec3 topRight = glm::normalize(reconstructCorner(tile + glm::vec2(1, 0)));
        glm::vec3 bottomRight = glm::normalize(reconstructCorner(tile + glm::vec2(1, 1)));
        glm::vec3 bottomLeft = glm::normalize(reconstructCorner(tile + glm::vec2(0, 1)));
        drawRay(origin, topLeft, 20.0f, {1, 0, 0});
        drawSphere(eye, 0.5f, {1, 1, 1});
        glm::vec3 nearTL = origin + topLeft * tileNear;
        glm::vec3 nearTR = origin + topRight * tileNear;
        glm::vec3 nearBR = origin + bottomRight * tileNear;
        glm::vec3 nearBL = origin + bottomLeft * tileNear;

        glm::vec3 farTL = origin + topLeft * tileFar;
        glm::vec3 farTR = origin + topRight * tileFar;
        glm::vec3 farBR = origin + bottomRight * tileFar;
        glm::vec3 farBL = origin + bottomLeft * tileFar;

        drawQuad(nearTL, nearTR, nearBR, nearBL, {1, 0, 0});
        drawQuad(farTL, farTR, farBR, farBL, {0, 1, 0});

        drawLine(nearTL, farTL, {0, 0, 1});
        drawLine(nearTR, farTR, {0, 0, 1});
        drawLine(nearBR, farBR, {0, 0, 1});
        drawLine(nearBL, farBL, {0, 0, 1});
    }
    void DebugDrawPass::execute(rhi::RHICommandBuffer *cmd, glm::mat4 &viewProj)
    {

        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());

        resource.vertexStorageBuffer->upload(m_vertices.data(), sizeof(DebugVertex) * m_vertices.size());
        cmd->bindPipeline(m_pipeline);
        cmd->bindDescriptorSet(resource.descriptorSet, 0);
        cmd->setPushConstant(&viewProj, sizeof(glm::mat4), 1);
        cmd->draw(getVertexCount());

        clear();
    };

} // namespace nitro::renderer
