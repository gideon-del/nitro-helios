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

    TransformCommand::TransformCommand(geometry::MeshTransformation oldTransformation, geometry::MeshTransformation newTransformation, MeshInstanceHandle handle)
        : m_oldTransformation(oldTransformation),
          m_newTransformation(newTransformation),
          m_handle(handle)
    {
    }

    void TransformCommand::undo(Scene &s)
    {
        auto instance = s.meshManager->getMeshInstance(m_handle);
        if (!instance)
            return;
        instance->transformation = m_oldTransformation;

        s.updateMeshInstance(m_handle);
    }
    void TransformCommand::execute(Scene &s)
    {
        auto instance = s.meshManager->getMeshInstance(m_handle);
        if (!instance)
            return;
        instance->transformation = m_newTransformation;

        s.updateMeshInstance(m_handle);
    }

    void DeleteMeshInstanceCommand::undo(Scene &s)
    {

        s.reactivateMeshInstanceSlot(m_handle, m_instance);
    };
    void DeleteMeshInstanceCommand::execute(Scene &s)
    {

        s.deactivateMeshInstanceSlot(m_handle);
    };

    void DeleteMeshInstanceCommand::OnDiscard(Scene &s)
    {

        s.reclaimMeshInstanceSlot(m_handle);
    };

    void CreateMeshInstanceCommand::undo(Scene &s)
    {

        s.deactivateMeshInstanceSlot(m_handle);
    };
    void CreateMeshInstanceCommand::execute(Scene &s)
    {
        if (m_handle.isValid())
        {
            s.reactivateMeshInstanceSlot(m_handle, m_instance);
            return;
        }
        m_handle = s.meshManager->addMeshInstances(m_instance);

        s.addMeshInstance(m_handle);
    };

    void CreateMeshInstanceCommand::OnDiscard(Scene &s)
    {
        s.reclaimMeshInstanceSlot(m_handle);
    };

} // namespace nitro::renderer
