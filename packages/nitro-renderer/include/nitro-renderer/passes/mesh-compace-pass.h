#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/render-graph.h>
#include <nitro-renderer/per-frame.h>

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

    struct MeshCompactResources
    {
        rhi::RHIDescriptorSet *descriptorSet;
        rhi::RHIBuffer *uniformBuffer;
        rhi::RHIBuffer *lastMeshDescriptorBuffer = nullptr;
        rhi::RHIBuffer *lastMeshInstanceBuffer = nullptr;
        rhi::RHIBuffer *lastSceneInstanceIdBuffer = nullptr;
        rhi::RHIBuffer *lastDrawCountBuffer = nullptr;
        rhi::RHIBuffer *lastDrawCommandBuffer = nullptr;
    };
    struct MeshCompactUBO
    {
        glm::mat4 viewProj;
    };
    class MeshCompactPass
    {
    public:
        MeshCompactPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal);
        ~MeshCompactPass();
        void execute(rhi::RHICommandBuffer *cmd, Scene &scene, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer, MeshCompactPushConstant &pc, MeshCompactUBO &ubo);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIComputePipeline *m_computePipeline;
        PerFrame<MeshCompactResources> m_resources;

        bool isSceneBufferStale(Scene &scene, MeshCompactResources &resource, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer);
        void bindSceneBuffer(Scene &scene, MeshCompactResources &resource, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer);
    };
} // namespace nitro::renderer
