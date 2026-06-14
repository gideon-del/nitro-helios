#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/per-frame.h>
#include <nitro-renderer/scene.h>
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
        DepthPrepass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal);
        ~DepthPrepass();
        rhi::RHITexture *getDepthTexture() { return m_depthTexture; };
        void resize(uint32_t width, uint32_t height);
        void execute(rhi::RHICommandBuffer *cmd, Scene &scene, DepthPrePassCamera camera);

    private:
        std::shared_ptr<rhi::RHIDevice>
            m_device;
        rhi::RHITexture *m_depthTexture;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIRenderPass *m_renderPass;
        PerFrame<DepthResource> m_resources;
        uint32_t m_width;
        uint32_t m_height;
    };
} // namespace nitro::renderer
