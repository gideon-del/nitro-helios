#pragma once
#include "nitro-rhi/rhi.h"
#include "nitro-renderer/per-frame.h"
#include "nitro-renderer/render-graph.h"
#include <glm/glm.hpp>
namespace nitro::renderer
{
    struct OcclusionCullRGResource
    {
        rhi::RHIBuffer *sceneDrawCount;
        rhi::RHIBuffer *sceneDrawCommands;
        rhi::RHIBuffer *hiZDrawCount;
        rhi::RHIBuffer *hiZDrawCommands;
        rhi::RHITexture *hizDepthTex;
    };
    struct OcclusionCullResource
    {
        rhi::RHIDescriptorSet *descriptorSet;
        rhi::RHIBuffer *lastMeshDescriptorBuffer = nullptr;
        rhi::RHIBuffer *lastMeshInstanceBuffer = nullptr;
        rhi::RHIBuffer *lastSceneDrawCountBuffer = nullptr;
        rhi::RHIBuffer *lastSceneDrawCommandBuffer = nullptr;
        rhi::RHIBuffer *lastHiZDrawCommandBuffer = nullptr;
        rhi::RHIBuffer *lastHiZDrawCountBuffer = nullptr;
        rhi::RHITexture *lastHizDepthTexture = nullptr;
    };
    struct OcclusionCullPushConstant
    {
        glm::mat4 view;
        glm::mat4 proj;
        float projScaleY;
        float depthScaleA;
        float screenHeight;
        int maxMip;
    };
    class OcclusionCullingPass
    {
    public:
        OcclusionCullingPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal);
        ~OcclusionCullingPass();
        void execute(rhi::RHICommandBuffer *cmd, OcclusionCullPushConstant &pc, Scene &scene, const OcclusionCullRGResource &rgResources);

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIComputePipeline *m_computePipeline;
        PerFrame<OcclusionCullResource> m_resources;
        bool isSceneStale(Scene &scene, OcclusionCullResource &resource, const OcclusionCullRGResource &rgResources);
        void bindSceneResources(Scene &scene, OcclusionCullResource &resource, const OcclusionCullRGResource &rgResources);
    };
} // namespace nitro::renderer
