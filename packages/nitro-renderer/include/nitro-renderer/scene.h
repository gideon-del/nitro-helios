#pragma once
#include "mesh-manager.h"
#include "material-manager.h"
#include <vector>
#include <nitro-rhi/rhi.h>
#include "spatial-grid.h"
#include "nitro-geometry/ray.h"
#include "editor-commands.h"
#include "nitro-assets/manager.h"
#include "gpu-resource-cache.h"
#include <filesystem>
namespace nitro::renderer
{

    namespace DebugColor
    {
        constexpr glm::vec3 Ray{1.00f, 0.90f, 0.10f};
        constexpr glm::vec3 Tested{0.35f, 0.35f, 0.38f};
        constexpr glm::vec3 Hit{1.00f, 0.45f, 0.05f};
        constexpr glm::vec3 Best{0.10f, 1.00f, 0.30f};
        constexpr glm::vec3 Selected{0.10f, 0.85f, 1.00f};
    }
    struct PickDebug
    {
        geometry::Ray ray;
        std::vector<MeshInstanceHandle> tested;
        std::vector<MeshInstanceHandle> hit;
        OptionalMeshInstanceHandle best;
    };

    struct SceneFrameResource
    {
        rhi::RHIBuffer *instanceIdBuffer = nullptr;
        size_t capacity = 1024;
    };
    struct Scene
    {

        Scene(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<MeshManager> meshManager, std::shared_ptr<MaterialManager> materialManager, std::shared_ptr<assets::AssetManager> assetManager)
            : m_device(std::move(device)), meshManager(std::move(meshManager)), materialManager(std::move(materialManager)),
              m_commands(EditorCommandStack(*this)),
              m_assetManager(std::move(assetManager))

        {
            m_sceneInstanceIdBuffers.create(
                g_MAX_FRAMES_IN_FLIGHT,
                [](uint32_t frameIdx)
                {
                    return SceneFrameResource{};
                });
        }

        ~Scene()
        {
            for (auto &resource : m_sceneInstanceIdBuffers)
            {
                if (resource.instanceIdBuffer)
                    m_device->destroyBuffer(resource.instanceIdBuffer);
            }
        }

        void draw(rhi::RHICommandBuffer *cmd, rhi::RHIBuffer *drawCommandsBuffer, rhi::RHIBuffer *drawCountBuffer)
        {

            cmd->bindVertexBuffer(meshManager->getVertexMegaBuffer());
            cmd->bindIndexBuffer(meshManager->getIndexMegaBuffer());

            cmd->drawIndexedIndirect(
                drawCommandsBuffer,
                0,
                static_cast<uint32_t>(m_instanceIds.size()),
                sizeof(rhi::DrawIndexedIndirectArgs));

            // cmd->drawIndexedIndirectCount(
            //     drawCommandsBuffer,
            //     0,
            //     drawCountBuffer,
            //     0,
            //     meshManager->instanceCount(),
            //     sizeof(rhi::DrawIndexedIndirectArgs));
        };
        void buildSceneInstanceId()
        {

            for (uint32_t i = 0; i < g_MAX_FRAMES_IN_FLIGHT; i++)
            {
                rebuildInstanceIdFrameBuffer(i);
            }

            m_dirtySceneInstanceIdBufferMask = 0;
        }
        rhi::RHIBuffer *sceneInstanceIdBuffer() const { return m_sceneInstanceIdBuffers.current(m_device->getCurrentFrameIndex()).instanceIdBuffer; }
        const std::vector<MeshInstanceHandle> &instanceIds() const { return m_instanceIds; }
        SpatialGrid &spatialGrid()
        {
            return m_grid;
        }

        const PickDebug &lastPick() const { return m_lastPick; }

        void setSelectedInstance(const MeshInstanceHandle &handle)
        {
            m_selectedInstance = handle;
        }

        EditorCommandStack &commands() { return m_commands; }
        std::shared_ptr<assets::AssetManager> assetManager() { return m_assetManager; }

        const OptionalMeshInstanceHandle &selectedInstance() const { return m_selectedInstance; }
        void loadGltfScene(std::string filePath, std::shared_ptr<rhi::RHIDevice> device);
        void addMeshInstance(const MeshInstanceHandle &handle);

        void updateMeshInstance(const MeshInstanceHandle &handle);
        void reclaimMeshInstanceSlot(const MeshInstanceHandle &handle);
        void reactivateMeshInstanceSlot(MeshInstanceHandle &handle, MeshInstance instance);
        void deactivateMeshInstanceSlot(const MeshInstanceHandle &handle);
        void pushCommand(std::unique_ptr<IEditorCommand> cmd);

        void flush();
        OptionalMeshInstanceHandle pickMeshInstance(const geometry::Ray &ray);
        void clear();
        void serialize(const std::filesystem::path &filepath);
        bool load(const std::filesystem::path &filepath);
        std::shared_ptr<MeshManager> meshManager;
        std::shared_ptr<MaterialManager> materialManager;

        static constexpr uint32_t s_MAX_DRAW_COMMANDS = 100000;

    private:
        PerFrame<SceneFrameResource> m_sceneInstanceIdBuffers;
        uint8_t m_dirtySceneInstanceIdBufferMask = 0;
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::vector<MeshInstanceHandle> m_instanceIds;
        SpatialGrid m_grid;
        OptionalMeshInstanceHandle m_selectedInstance;
        PickDebug m_lastPick;
        EditorCommandStack m_commands;
        std::shared_ptr<assets::AssetManager> m_assetManager;

        void rebuildInstanceIdFrameBuffer(uint32_t frameIdx);
        void markInstanceIdBuffersDirty()
        {
            m_dirtySceneInstanceIdBufferMask = (1 << g_MAX_FRAMES_IN_FLIGHT) - 1;
        }
    };
} // namespace nitro::renderer
