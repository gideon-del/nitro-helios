#pragma once
#include <glm/glm.hpp>
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/per-frame.h>

namespace nitro::renderer
{

    struct DebugVertex
    {
        glm::vec4 position;
        glm::vec4 color;
    };
    struct DebugPassResource
    {
        rhi::RHIBuffer *vertexStorageBuffer;
        rhi::RHIDescriptorSet *descriptorSet;
    };
    struct TileFrustumCPU
    {
        glm::vec3 topNormal;
        glm::vec3 bottomNormal;
        glm::vec3 leftNormal;
        glm::vec3 rightNormal;
        float tileNear;
        float tileFar;
    };

    class DebugDrawPass
    {
        static constexpr size_t MAX_LINES = 100000;

    public:
        DebugDrawPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal);
        ~DebugDrawPass();
        void drawLine(glm::vec3 start, glm::vec3 end, glm::vec3 color = {0.0, 0.0, 0.0});
        void drawRay(glm::vec3 origin, glm::vec3 direction, float length, glm::vec3 color);
        void drawAxes(const glm::mat4 &transform, float scale);
        void drawQuad(glm::vec3 c0, glm::vec3 c1, glm::vec3 c2, glm::vec3 c3, glm::vec3 color);
        void drawPlane(
            const glm::vec3 &point,
            const glm::vec3 &normal,
            float size,
            const glm::vec4 color);
        void drawAABB(glm::vec3 min, glm::vec3 max, glm::vec3 color);
        void drawFrustum(glm::mat4 invViewProj, float near, float far, glm::vec3 color);
        void drawTileFrustum(glm::vec2 tile, glm::vec2 screenSize,
                             const glm::mat4 &invProj, const glm::mat4 &view, float tileNear,
                             float tileFar);
        void drawSphere(glm::vec3 center, float radius, glm::vec3 color);
        void drawCircle(
            glm::vec3 center,
            glm::vec3 axisA,
            glm::vec3 axisB,
            float radius,
            glm::vec3 color);
        void execute(rhi::RHICommandBuffer *cmd, glm::mat4 &viewProj);

        void clear() { m_vertices.clear(); };
        uint32_t getVertexCount() const { return static_cast<uint32_t>(m_vertices.size()); };

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        uint32_t m_width, m_height;
        std::vector<DebugVertex> m_vertices;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIPipeline *m_pipeline;
        PerFrame<DebugPassResource> m_resources;
    };
} // namespace nitro::renderer
