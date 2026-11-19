#include "nitro-studio/application.h"
#include "nitro-studio/device-factory.h"

#include "nitro-renderer/render-pipeline/clear-pipeline.h"
namespace nitro::studio
{

    static GLFWwindow *createWindow(int width, int height, const char *title)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow *window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window)
            throw std::runtime_error("Failed to create window");
        return window;
    }

    Application::Application()
        : m_window(createWindow(800, 600, "Nitro Studio")),
          m_device(createDevice(defaultBackend(), m_window.get())),
          m_renderer(*m_device.get()),
          m_swapchain(m_device->createSwapchain(nullptr)),
          m_assetManager(std::make_shared<assets::AssetManager>()),
          m_entityStore(std::make_shared<renderer::EntityStore>()),
          m_gpuResourceCache(std::make_shared<renderer::GPUResourceCache>(m_device)),
          m_meshManager(std::make_shared<renderer::MeshManager>(m_device, m_assetManager, m_entityStore)),
          m_materialManager(std::make_shared<renderer::MaterialManager>(m_device, m_assetManager, m_gpuResourceCache)),
          m_lightManager(std::make_shared<renderer::LightManager>(m_device, m_entityStore)),
          m_scene(m_device, m_meshManager, m_materialManager, m_assetManager, m_entityStore, m_lightManager)

    {

        m_view = m_renderer.createView({800, 600}, std::make_unique<renderer::ClearPipeline>());
        m_viewportExtent = {800, 600};
        int w, h;
        glfwGetFramebufferSize(m_window.get(), &w, &h);

        m_swapchainExtent = {(uint32_t)w, (uint32_t)h};
        m_frameClock.time = glfwGetTime();
    }

    Application::~Application()
    {
        m_device->waitIdle();
    }

    void Application::run()
    {

        while (!glfwWindowShouldClose(m_window.get()))
        {
            glfwPollEvents();

            m_frameClock.tick(glfwGetTime());
            handleResize();
            renderFrame();
        }
    }

    void Application::handleResize()
    {

        if (m_pendingViewResize)
        {
            m_renderer.resize(m_view, m_pendingViewResize.value());
            m_pendingViewResize = std::nullopt;
        }
        int w, h;
        glfwGetFramebufferSize(m_window.get(), &w, &h);

        if (w == 0 || h == 0)
            return;
        Extent2D extent{uint32_t(w), uint32_t(h)};

        if (extent == m_swapchainExtent)
            return;

        m_device->waitIdle();
        m_swapchain->resize(extent.width, extent.height);
        m_swapchainExtent = extent;
    }

    void Application::renderFrame()
    {

        auto *cmd = m_device->beginFrame();

        frame::FrameInfo frameInfo{
            .cmd = *cmd,
            .frameIndex = m_device->getCurrentFrameIndex(),
            .frameNumber = m_frameClock.frameNumber,

        };

        renderer::RenderRequest request{};

        auto sceneTexture = m_renderer.render(m_view, request, frameInfo);

        rhi::RHIRenderPassDesc rpDesc{};
        rpDesc.clearColor[0] = 0.0f;
        rpDesc.clearColor[1] = 1.0f;
        rpDesc.clearColor[2] = 1.0f;
        rpDesc.clearColor[3] = 1.0f;
        rpDesc.clearDepth = 1.0f;
        rpDesc.hasDepth = true;

        cmd->beginRenderPass(rpDesc);

        m_device->beginImGuiFrame();
        ImGui::DockSpaceOverViewport();
        ImGui::Begin("Viewport");
        // ImGui::Text("dt: %.3f ms", m_frameClock.dt * 1000.0f);

        ImVec2 size = ImGui::GetContentRegionAvail();

        ImGui::Image((ImTextureID)m_device->getImGuiTextureRef(m_device->resolve(sceneTexture)), size);

        float dpi = ImGui::GetWindowViewport()->DpiScale;
        Extent2D pixels{uint32_t(size.x * dpi), uint32_t(size.y * dpi)};
        if (pixels != m_viewportExtent && pixels.width >= 8 && pixels.height >= 8)
        {
            m_pendingViewResize = pixels;
            m_viewportExtent = pixels;
        }

        ImGui::End();

        m_device->endImGuiFrame();
        m_device->drawImGui(cmd);
        cmd->endRenderPass();

        cmd->present();
        m_device->endFrame(cmd);
    }
} // namespace nitro::studio
