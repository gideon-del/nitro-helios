#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/render-graph.h>
#include <nitro-renderer/per-frame.h>

namespace nitro::renderer
{

    struct MeshCompactPushConstant
    {
        glm::mat4 view;
        glm::mat4 proj;
        uint32_t objectCount;
        float projScaleY;
        float screenHeight;
        int maxMip;
        uint lodEnabled = 1;
        uint frustumCullEnabled = 1;
        uint occlusionCullEnabled = 1;
        float _pads;
    };

    struct MeshCompactBuffers
    {
        rhi::RHIBuffer *meshDescriptorBuffer;
        rhi::RHIBuffer *meshInstanceBuffer;
        RGBufferID drawCountID;
        RGBufferID drawCommandsID;
        RGTextureID hizTexture;
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
        rhi::RHITexture *lastHizTexture = nullptr;
    };
    struct MeshCompactUBO
    {
        glm::mat4 viewProj;
        float projScaleY;
        float _pads[3];
    };
    class MeshCompactPass
    {
    public:
        MeshCompactPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal);
        ~MeshCompactPass();
        void execute(rhi::RHICommandBuffer *cmd, Scene &scene, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer, MeshCompactPushConstant &pc, rhi::RHITexture *hizTexture);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIComputePipeline *m_computePipeline;
        PerFrame<MeshCompactResources> m_resources;

        bool isSceneBufferStale(Scene &scene, MeshCompactResources &resource, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer, rhi::RHITexture *hizTexture);
        void bindSceneBuffer(Scene &scene, MeshCompactResources &resource, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer, rhi::RHITexture *hizTexture);
    };
} // namespace nitro::renderer
