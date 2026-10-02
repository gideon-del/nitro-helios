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
    class DeleteEntityCommand : public IEditorCommand
    {
        EntityHandle m_handle;
        Entity m_entity;
        std::optional<MeshInstance> m_meshInstance;
        std::optional<PointLight> m_pointLight;

    public:
        DeleteEntityCommand(EntityHandle handle, Entity entity) : m_handle(handle), m_entity(entity)
        {
        }
        void undo(Scene &s) override;
        void execute(Scene &s) override;
        void OnDiscard(Scene &s) override;
        const char *name() const override { return "Delete Entity Instance"; }
    };
    class CreateMeshInstanceCommand : public IEditorCommand
    {
        EntityHandle m_handle{};
        GPUMeshHandle m_meshHandle;
        GPUMaterialHandle m_materialHandle;
        geometry::MeshTransformation m_transformation;
        std::optional<Entity> m_entity;
        std::optional<MeshInstance> m_meshInstance;

    public:
        CreateMeshInstanceCommand(GPUMeshHandle mesh, GPUMaterialHandle material, geometry::MeshTransformation transformation) : m_meshHandle(mesh), m_materialHandle(material), m_transformation(transformation)
        {
        }
        void undo(Scene &s) override;
        void execute(Scene &s) override;
        void OnDiscard(Scene &s) override;
        const char *name() const override { return "Create Mesh Instance"; }
    };
    class CreatePointLightCommand : public IEditorCommand
    {
        EntityHandle m_handle{};
        float m_radius;
        float m_intensity;
        glm::vec3 m_color;

        std::optional<Entity> m_entity;
        std::optional<PointLight> m_pointLight;

    public:
        CreatePointLightCommand(float radius = 20.0f, float intensity = 3.0f, glm::vec3 color = glm::vec3{1.0f}) : m_radius(radius), m_intensity(intensity), m_color(color)
        {
        }
        void undo(Scene &s) override;
        void execute(Scene &s) override;
        void OnDiscard(Scene &s) override;
        const char *name() const override { return "Create Point Light"; }
    };

} // namespace nitro::renderer
