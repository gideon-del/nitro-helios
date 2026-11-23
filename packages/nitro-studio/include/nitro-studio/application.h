#pragma once
#include "nitro-rhi/rhi-device.h"
#include "nitro-renderer/scene/light-manager.h"
#include "nitro-renderer/scene/mesh-manager.h"
#include "nitro-renderer/scene/material-manager.h"
#include "nitro-renderer/scene/entity-store.h"
#include "nitro-renderer/scene/scene.h"
#include "nitro-assets/manager.h"
#include "nitro-geometry/camera.h"
#include "nitro-core/types/size.h"
#include "nitro-renderer/cache/gpu-resource-cache.h"
#include "nitro-studio/camera-controller.h"
#include "nitro-renderer/scene-renderer.h"
#include "nitro-studio/device-factory.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <filesystem>

namespace nitro::studio
{
    struct GlfwContext
    {
        GlfwContext()
        {
            if (!glfwInit())
                throw std::runtime_error("GLFW init failed");
        }
        ~GlfwContext() { glfwTerminate(); }
    };
    struct WindowDeleter
    {
        void operator()(GLFWwindow *w) const { glfwDestroyWindow(w); }
    };

    struct FrameClock
    {
        double time = 0.0;
        float dt = 0.0f;
        uint64_t frameNumber = 0;

        void tick(double now)
        {
            dt = static_cast<float>(std::min(now - time, 0.1));
            time = now;
            ++frameNumber;
        }
    };

    renderer::CameraData toCameraData(const geometry::OrbitalCamera &camera);
    class Application
    {

    public:
        Application();
        ~Application();

        void run();

        Application(const Application &) = delete;
        Application &operator=(const Application &) = delete;

    private:
        void handleInput();
        void handleResize();
        void renderFrame();
        void flush();
        void handleCameraInput();
        void scanEnvironments();

        void destroyEnvironment();

        void bakeEnvironment(const std::filesystem::path &path);

    private:
        GlfwContext m_glfw;
        std::unique_ptr<GLFWwindow, WindowDeleter> m_window;

        const Backend m_backend;

        std::shared_ptr<rhi::RHIDevice> m_device;
        renderer::SceneRenderer m_renderer;
        std::shared_ptr<rhi::RHISwapchain> m_swapchain;

        std::shared_ptr<assets::AssetManager> m_assetManager;
        std::shared_ptr<renderer::EntityStore> m_entityStore;

        std::shared_ptr<renderer::GPUResourceCache> m_gpuResourceCache;
        std::shared_ptr<renderer::MeshManager> m_meshManager;
        std::shared_ptr<renderer::MaterialManager> m_materialManager;
        std::shared_ptr<renderer::LightManager> m_lightManager;

        renderer::Scene m_scene;

        geometry::OrbitalCamera m_editorCamera;

        CameraController m_cameraController;
        FrameClock m_frameClock;

        Extent2D m_swapchainExtent;
        frame::RenderViewHandle m_view;

        std::optional<Extent2D> m_pendingViewResize = std::nullopt;
        Extent2D m_viewportExtent;
        renderer::SceneRenderData m_sceneData;
        renderer::PipelineSettings m_pipelineSettings{};
        renderer::PostSettings m_postSettings;

        bool m_viewportHovered = false;
        bool m_viewportFocused = false;
        bool m_cameraDragging = false;

        renderer::DirectionalLight m_sun{
            .direction = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.5f)),
            .color = glm::vec3(1.0f),
            .intensity = 3.0f,
        };
        renderer::EnvironmentData m_environment{};

        std::vector<std::filesystem::path> m_hdrFiles;
        int m_selectedHdr = 0;

        bool m_pendingEnvironment = false;
    };
} // namespace nitro::studio
