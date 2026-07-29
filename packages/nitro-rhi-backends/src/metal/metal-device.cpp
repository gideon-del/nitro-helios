#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-command-buffer.h>
#include <nitro-rhi-backends/metal/metal-swapchain.h>
#include <nitro-rhi-backends/metal/metal-pipeline.h>
#include <nitro-rhi-backends/metal/metal-buffer.h>
#include <nitro-rhi-backends/metal/metal-texture.h>
#include <nitro-rhi-backends/metal/metal-descriptor-layout.h>
#include <nitro-rhi-backends/metal/metal-descriptor-set.h>
#include <nitro-rhi-backends/metal/metal-render-pass.h>
#include <nitro-rhi-backends/metal/metal-descriptor-layout.h>
#include <nitro-rhi-backends/metal/metal-timer.h>
#include <nitro-rhi-backends/metal/metal-compute-pipeline.h>
#include <imgui_impl_glfw.h>
#ifndef IMGUI_IMPL_METAL_CPP
#define IMGUI_IMPL_METAL_CPP
#endif
#include <imgui_impl_metal.h>
#include <imnodes.h>
#include <GLFW/glfw3.h>
namespace nitro::rhi::metal
{
    MetalDevice::MetalDevice(void *window) : m_window(window)
    {
        device = MTL::CreateSystemDefaultDevice();
        commandQueue = device->newCommandQueue();

        m_defaultSamplers.anisotropicRepeat = create(RHISamplerDesc::AnisotropicRepeat());
        m_defaultSamplers.linearClamp = create(RHISamplerDesc::LinearClamp());
        m_defaultSamplers.linearRepeat = create(RHISamplerDesc::LinearRepeat());
        m_defaultSamplers.nearestClamp = create(RHISamplerDesc::NearestClamp());
        m_defaultSamplers.nearestRepeat = create(RHISamplerDesc::NearestRepeat());
        m_defaultSamplers.shadow = create(RHISamplerDesc::Shadow());

        float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(main_scale);
        style.FontScaleDpi = main_scale;

        GLFWwindow *glfwWindow = reinterpret_cast<GLFWwindow *>(window);
        ImGui_ImplGlfw_InitForOther(glfwWindow, true);
        ImGui_ImplMetal_Init(device);
        ImNodes::CreateContext();
    }
    MetalDevice::~MetalDevice()
    {
        commandQueue->release();

        ImNodes::DestroyContext();
        ImGui::DestroyContext();
        device->release();
    }

    RHIBuffer *MetalDevice::createBuffer(const BufferDesc &desc)
    {
        return new MetalBuffer(this, desc);
    }
    RHITexture *MetalDevice::createTexture(const TextureDesc &desc)
    {
        return new MetalTexture(this, desc);
    }
    RHIDescriptorLayout *MetalDevice::createDescriptorLayout(const std::vector<RHIDescriptorBinding> bindings)
    {
        return new MetalDescriptorLayout(this, bindings);
    }
    void MetalDevice::destroyDescriptorLayout(RHIDescriptorLayout *layout)
    {
        delete layout;
    };
    RHIDescriptorSet *MetalDevice::createDescriptorSet(RHIDescriptorLayout *layout)
    {
        MetalDescriptorLayout *metalLayout = reinterpret_cast<MetalDescriptorLayout *>(layout);
        return new MetalDescriptorSet(this, metalLayout);
    }
    void MetalDevice::destroyDescriptorSet(RHIDescriptorSet *set)
    {
        delete set;
    }
    RHIRenderPass *MetalDevice::createRenderPass(const RenderPassDesc &desc)
    {
        return new MetalRenderPass(this, desc);
    };
    void MetalDevice::destroyRenderPass(RHIRenderPass *renderPass)
    {
        delete renderPass;
    }
    RHIPipeline *MetalDevice::createPipeline(const PipelineDesc &desc)
    {
        return new MetalPipeline(this, desc);
    }

    RHIComputePipeline *MetalDevice::createComputePipeline(const ComputePipelineDesc &desc)
    {
        return new MetalComputePipeline(this, desc);
    }

    void MetalDevice::destroyComputePipeline(RHIComputePipeline *pipeline)
    {
        delete pipeline;
    }
    RHITimer *MetalDevice::createTimer()
    {
        return new MetalTimer(this);
    }

    void MetalDevice::destroyBuffer(RHIBuffer *buffer)
    {
        delete buffer;
    }

    void MetalDevice::destroyTexture(RHITexture *texture)
    {
        delete texture;
    }
    void MetalDevice::destroyPipeline(RHIPipeline *pipeline)
    {
        delete pipeline;
    }

    RHISwapchain *MetalDevice::createSwapchain(RHISurface *surface)
    {
        MetalSwapchain *swapchain = new MetalSwapchain(this, m_window);

        m_swapchain = swapchain;

        return swapchain;
    }
    RHICommandBuffer *MetalDevice::beginFrame()
    {
        if (!m_swapchain)
        {
            throw std::runtime_error("Metal Swapchain not found");
        }
        if (m_swapchain->currentDrawable)
        {
            m_swapchain->currentDrawable->release();
        }
        m_swapchain->currentDrawable = m_swapchain->layer->nextDrawable();
        m_currentCommandBuffer = new MetalCommandBuffer(this, m_swapchain);
        return m_currentCommandBuffer;
    }
    RHICommandBuffer *MetalDevice::createCommandBuffer()
    {

        m_currentCommandBuffer = new MetalCommandBuffer(this);
        return m_currentCommandBuffer;
    }

    RHISamplerHandle MetalDevice::create(const RHISamplerDesc &desc)
    {
        return RHISamplerHandle{};
    };
    void MetalDevice::destroy(RHISamplerHandle &handle)
    {
    }
    void MetalDevice::endFrame(RHICommandBuffer *cmd)
    {
        MetalCommandBuffer *metalCmd = reinterpret_cast<MetalCommandBuffer *>(cmd);

        metalCmd->commandBuffer->commit();
        delete cmd;
    }
    void MetalDevice::endCommandBuffer(RHICommandBuffer *cmd)
    {
        MetalCommandBuffer *metalCmd = reinterpret_cast<MetalCommandBuffer *>(cmd);
        metalCmd->endEncoders();
        metalCmd->commandBuffer->commit();
        metalCmd->commandBuffer->waitUntilCompleted();
        delete cmd;
    }
    void MetalDevice::waitIdle() {};

    uint32_t MetalDevice::getCurrentFrameIndex() const
    {
        return 0;
    }
    void MetalDevice::beginImGuiFrame()
    {
        if (!m_currentCommandBuffer)
        {
            throw std::runtime_error("Must begin frame first befor imgui frame");
        }
        if (!m_currentCommandBuffer->rpd)
        {
            throw std::runtime_error("Must call in main render pass");
        }
        ImGui_ImplMetal_NewFrame(m_currentCommandBuffer->rpd);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }
    void MetalDevice::endImGuiFrame()
    {
        ImGui::Render();
    }
    void MetalDevice::drawImGui(RHICommandBuffer *cmd)
    {
        MetalCommandBuffer *metalCmd = reinterpret_cast<MetalCommandBuffer *>(cmd);

        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), metalCmd->commandBuffer, metalCmd->encoder);
    }

    void *MetalDevice::getImGuiTextureRef(RHITexture *texture)
    {
        auto *mtlTex = static_cast<MetalTexture *>(texture);
        return (void *)mtlTex->texture;
    }
} // namespace nitro::rhi::metal
