#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/render-graph.h>

namespace nitro::renderer
{

    struct MeshCompactPushConstant
    {
        uint32_t objectCount;
        uint32_t indexSize;
        uint32_t vertexSize;
        float _pad;
    };

    struct MeshCompactBuffers
    {
        rhi::RHIBuffer *meshDescriptorBuffer;
        rhi::RHIBuffer *meshInstanceBuffer;
        RGBufferID drawCountID;
        RGBufferID drawCommandsID;
    };
    class MeshCompactPass
    {
    public:
        MeshCompactPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal);
        ~MeshCompactPass();
        void execute(rhi::RHICommandBuffer *cmd, Scene &scene, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer, MeshCompactPushConstant &pc);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIComputePipeline *m_computePipeline;
        rhi::RHIDescriptorSet *m_descriptorSet;
        rhi::RHIBuffer *m_lastMeshDescriptorBuffer = nullptr;
        rhi::RHIBuffer *m_lastMeshInstanceBuffer = nullptr;
        rhi::RHIBuffer *m_lastSceneInstanceIdBuffer = nullptr;

        bool isSceneBufferStale(Scene &scene);
        void bindSceneBuffer(Scene &scene, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer);
    };
} // namespace nitro::renderer
