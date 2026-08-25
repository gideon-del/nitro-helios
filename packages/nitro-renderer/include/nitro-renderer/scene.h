#pragma once
#include "mesh-manager.h"
#include "material-manager.h"
#include <vector>
#include <nitro-rhi/rhi.h>
#include "spatial-grid.h"
#include "nitro-geometry/ray.h"
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
    struct Scene
    {

        Scene(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<MeshManager> meshManager, std::shared_ptr<MaterialManager> materialManager)
            : m_device(std::move(device)), meshManager(std::move(meshManager)), materialManager(std::move(materialManager)) {}

        ~Scene()
        {
            if (m_sceneInstanceIdBuffer)
                m_device->destroyBuffer(m_sceneInstanceIdBuffer);
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
            std::vector<uint32_t> gpuInstanceIds;

            gpuInstanceIds.reserve(m_instanceIds.size());

            for (auto &handle : m_instanceIds)
            {
                gpuInstanceIds.push_back(handle.id);
            }
            rhi::BufferDesc desc;
            desc.initialData = gpuInstanceIds.data();
            desc.size = sizeof(uint32_t) * gpuInstanceIds.size();
            desc.storage = rhi::BufferDesc::StorageMode::GPU;
            desc.usage = rhi::BufferDesc::Usage::Storage | rhi::BufferDesc::Usage::TransferDst;

            if (m_sceneInstanceIdBuffer)
                m_device->destroyBuffer(m_sceneInstanceIdBuffer);
            m_sceneInstanceIdBuffer = m_device->createBuffer(desc);
        }
        rhi::RHIBuffer *sceneInstanceIdBuffer() const { return m_sceneInstanceIdBuffer; }
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

        const OptionalMeshInstanceHandle &selectedInstance() const { return m_selectedInstance; }
        void loadGltfScene(std::string filePath, std::shared_ptr<rhi::RHIDevice> device);
        void addMeshInstance(const MeshInstanceHandle &handle);

        void updateMeshInstance(const MeshInstanceHandle &handle);
        OptionalMeshInstanceHandle pickMeshInstance(const geometry::Ray &ray);
        std::shared_ptr<MeshManager> meshManager;
        std::shared_ptr<MaterialManager> materialManager;

        static constexpr uint32_t s_MAX_DRAW_COMMANDS = 100000;

    private:
        rhi::RHIBuffer *m_sceneInstanceIdBuffer = nullptr;
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::vector<MeshInstanceHandle> m_instanceIds;
        SpatialGrid m_grid;
        OptionalMeshInstanceHandle m_selectedInstance;
        PickDebug m_lastPick;
    };
} // namespace nitro::renderer
