#include "nitro-renderer/editor-commands.h"
#include "nitro-renderer/scene.h"

namespace nitro::renderer
{

    void EditorCommandStack::push(Command cmd)
    {
        cmd->execute(m_scene);
        m_done.push_back(std::move(cmd));
        for (auto &c : m_undone)
            c->OnDiscard(m_scene);
        m_undone.clear();
        if (m_done.size() > MAX_COMMANDS)
        {
            m_done.front()->OnDiscard(m_scene);
            m_done.erase(m_done.begin());
        }
    };

    void EditorCommandStack::undo()
    {
        if (!canUndo())
            return;

        auto lastCmd = std::move(m_done.back());
        m_done.pop_back();

        lastCmd->undo(m_scene);
        m_undone.push_back(std::move(lastCmd));
    };

    void EditorCommandStack::redo()
    {
        if (!canRedo())
            return;
        auto lastCmd = std::move(m_undone.back());
        m_undone.pop_back();
        lastCmd->execute(m_scene);
        m_done.push_back(std::move(lastCmd));
    }

    bool EditorCommandStack::canUndo() const
    {
        return !m_done.empty();
    }

    bool EditorCommandStack::canRedo() const
    {
        return !m_undone.empty();
    }
    void EditorCommandStack::clear()
    {
        m_done.clear();
        m_undone.clear();
    };

    TransformCommand::TransformCommand(geometry::MeshTransformation oldTransformation, geometry::MeshTransformation newTransformation, EntityHandle handle)
        : m_oldTransformation(oldTransformation),
          m_newTransformation(newTransformation),
          m_handle(handle)
    {
    }

    void TransformCommand::undo(Scene &s)
    {
        auto entity = s.entityStore()->get(m_handle);
        if (!entity)
            return;
        entity->transformation = m_oldTransformation;

        s.updateEntity(m_handle);
    }
    void TransformCommand::execute(Scene &s)
    {
        auto entity = s.entityStore()->get(m_handle);
        if (!entity)
            return;
        entity->transformation = m_newTransformation;

        s.updateEntity(m_handle);
    }

    void DeleteEntityCommand::undo(Scene &s)
    {

        s.reactivateEntitySlot(m_handle, m_entity, m_meshInstance, m_pointLight);
    };
    void DeleteEntityCommand::execute(Scene &s)
    {

        if (m_entity.pointLight)
        {
            auto light = s.lightManager()->getPointLight(*m_entity.pointLight);
            if (light)
            {
                m_pointLight = *light;
            }
        }
        if (m_entity.meshInstance)
        {
            auto meshInstance = s.meshManager->getMeshInstance(*m_entity.meshInstance);
            if (meshInstance)
            {
                m_meshInstance = *meshInstance;
            }
        }
        s.deactivateEntitySlot(m_handle);
    };

    void DeleteEntityCommand::OnDiscard(Scene &s)
    {

        s.reclaimEntitySlot(m_handle);
    };

    void CreateMeshInstanceCommand::undo(Scene &s)
    {
        s.deactivateEntitySlot(m_handle);
    };
    void CreateMeshInstanceCommand::execute(Scene &s)
    {
        if (m_handle.isValid() && m_entity)
        {
            s.reactivateEntitySlot(m_handle, *m_entity, m_meshInstance, std::nullopt);
            return;
        }
        m_handle = s.addMeshEntity(m_meshHandle, m_materialHandle, m_transformation, "Entity");
        auto entity = s.entityStore()->get(m_handle);

        m_entity = *entity;

        auto meshInstance = s.meshManager->getMeshInstance(*entity->meshInstance);

        m_meshInstance = *meshInstance;
    };

    void CreateMeshInstanceCommand::OnDiscard(Scene &s)
    {
        s.reclaimEntitySlot(m_handle);
    };

    void CreatePointLightCommand::undo(Scene &s)
    {
        s.deactivateEntitySlot(m_handle);
    };
    void CreatePointLightCommand::execute(Scene &s)
    {
        if (m_handle.isValid() && m_entity)
        {
            s.reactivateEntitySlot(m_handle, *m_entity, std::nullopt, m_pointLight);
            return;
        }
        m_handle = s.addPointLightEntity(m_radius, m_intensity, m_color);
        auto entity = s.entityStore()->get(m_handle);

        m_entity = *entity;

        auto pointLight = s.lightManager()->getPointLight(*entity->pointLight);

        m_pointLight = *pointLight;
    };

    void CreatePointLightCommand::OnDiscard(Scene &s)
    {
        s.reclaimEntitySlot(m_handle);
    };

} // namespace nitro::renderer
