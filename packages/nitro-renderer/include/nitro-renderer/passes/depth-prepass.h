#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/per-frame.h>
#include <nitro-renderer/scene.h>
#include <nitro-renderer/render-graph.h>
#include <glm/glm.hpp>
namespace nitro::renderer
{
    struct DepthResource
    {
        rhi::RHIBuffer *uniformBuffer;
        rhi::RHIDescriptorSet *descriptorSet;
    };

    struct DepthPrePassCamera
    {
        glm::mat4 view;
        glm::mat4 proj;
    };
    class DepthPrepass
    {
    public:
        DepthPrepass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal, std::shared_ptr<MaterialSystem> materialSystem);
        ~DepthPrepass();
        void resize(uint32_t width, uint32_t height);
        void execute(rhi::RHICommandBuffer *cmd, Scene &scene, DepthPrePassCamera camera);
        void bindResources(const RGResources &resource, RGTextureID depth);

    private:
        std::shared_ptr<rhi::RHIDevice>
            m_device;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIRenderPass *m_renderPass = nullptr;
        PerFrame<DepthResource> m_resources;
        uint32_t m_width;
        uint32_t m_height;
        std::shared_ptr<MaterialSystem> m_materialSystem;
    };
} // namespace nitro::renderer
