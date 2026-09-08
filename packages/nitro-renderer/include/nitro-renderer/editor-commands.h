#pragma once
#include "mesh-manager.h"

namespace nitro::renderer
{

    struct Scene;

    class IEditorCommand
    {
    public:
        virtual const char *name() const = 0;
        virtual void undo(Scene &s) = 0;
        virtual void execute(Scene &s) = 0;
        virtual void OnDiscard(Scene &s) {};
        virtual ~IEditorCommand() = default;
    };

    class EditorCommandStack
    {
        using Command = std::unique_ptr<IEditorCommand>;
        std::vector<Command> m_done;
        std::vector<Command> m_undone;
        static constexpr uint32_t MAX_COMMANDS = 150;
        Scene &m_scene;

    public:
        EditorCommandStack(Scene &scene) : m_scene(scene) {};
        void push(Command cmd);
        void undo();
        void redo();
        bool canUndo() const;
        bool canRedo() const;
        size_t undoSize() const { return m_done.size(); }
        size_t redoSize() const { return m_undone.size(); }
        void clear();
    };

    class TransformCommand : public IEditorCommand
    {

        geometry::MeshTransformation m_oldTransformation;
        geometry::MeshTransformation m_newTransformation;
        EntityHandle m_handle;

    public:
        TransformCommand(geometry::MeshTransformation oldTransformation, geometry::MeshTransformation newTransformation, EntityHandle handle);
        ~TransformCommand() override = default;
        void undo(Scene &s) override;
        void execute(Scene &s) override;
        const char *name() const override { return "Transform"; }
    };
    class DeleteMeshInstanceCommand : public IEditorCommand
    {
        EntityHandle m_handle;
        Entity m_entity;

    public:
        DeleteMeshInstanceCommand(EntityHandle handle, Entity entity) : m_handle(handle), m_entity(entity)
        {
        }
        void undo(Scene &s) override;
        void execute(Scene &s) override;
        void OnDiscard(Scene &s) override;
        const char *name() const override { return "Delete Mesh Instance"; }
    };
    class CreateMeshInstanceCommand : public IEditorCommand
    {
        EntityHandle m_handle{};
        GPUMeshHandle m_meshHandle;
        GPUMaterialHandle m_materialHandle;
        geometry::MeshTransformation m_transformation;
        std::optional<Entity> m_entity;

    public:
        CreateMeshInstanceCommand(GPUMeshHandle mesh, GPUMaterialHandle material, geometry::MeshTransformation transformation) : m_meshHandle(mesh), m_materialHandle(material), m_transformation(transformation)
        {
        }
        void undo(Scene &s) override;
        void execute(Scene &s) override;
        void OnDiscard(Scene &s) override;
        const char *name() const override { return "Create Mesh Instance"; }
    };

} // namespace nitro::renderer
