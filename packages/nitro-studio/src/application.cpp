#include "nitro-studio/application.h"
#include "nitro-studio/device-factory.h"

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
          m_swapchain(m_device->createSwapchain(nullptr)),
          m_assetManager(std::make_shared<assets::AssetManager>()),
          m_entityStore(std::make_shared<renderer::EntityStore>()),
          m_gpuResourceCache(std::make_shared<renderer::GPUResourceCache>(m_device)),
          m_meshManager(std::make_shared<renderer::MeshManager>(m_device, m_assetManager, m_entityStore)),
          m_materialManager(std::make_shared<renderer::MaterialManager>(m_device, m_assetManager, m_gpuResourceCache)),
          m_lightManager(std::make_shared<renderer::LightManager>(m_device, m_entityStore)),
          m_scene(m_device, m_meshManager, m_materialManager, m_assetManager, m_entityStore, m_lightManager)

    {
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

            renderFrame();
        }
    }

    void Application::renderFrame()
    {
        auto *cmd = m_device->beginFrame();

        rhi::RHIRenderPassDesc rpDesc{};
        rpDesc.clearColor[0] = 0.0f;
        rpDesc.clearColor[1] = 1.0f;
        rpDesc.clearColor[2] = 1.0f;
        rpDesc.clearColor[3] = 1.0f;
        rpDesc.clearDepth = 1.0f;
        rpDesc.hasDepth = true;

        cmd->beginRenderPass(rpDesc);

        cmd->endRenderPass();

        cmd->present();
        m_device->endFrame(cmd);
    }
} // namespace nitro::studio
