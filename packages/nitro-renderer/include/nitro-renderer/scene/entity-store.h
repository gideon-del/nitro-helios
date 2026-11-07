#pragma once
#include "handles.h"
#include "nitro-core/types/pool.h"
#include <functional>

namespace nitro::renderer
{
    class EntityStore
    {
        ResourcePool<Entity> m_pool;
        std::unordered_map<EntityID, EntityHandle> m_byId;

    public:
        EntityHandle create(std::string name = "Entity");
        EntityHandle createWithId(EntityID id, std::string name);
        Entity *get(EntityHandle h);
        std::optional<EntityHandle> find(EntityID id) const;
        void destroy(EntityHandle h);
        void reclaimSlot(EntityHandle h);
        void deactivateSlot(EntityHandle h);
        bool reactivateSlot(EntityHandle h, Entity e);
        void forEach(std::function<void(const Entity &entity)> callback);
        void clear();
    };
} // namespace nitro::renderer
