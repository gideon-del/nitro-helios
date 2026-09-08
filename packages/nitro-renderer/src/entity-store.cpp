#include "nitro-renderer/entity-store.h"

namespace nitro::renderer
{
    EntityHandle EntityStore::create(std::string name)
    {
        return createWithId(EntityID::generate(), name);
    }
    EntityHandle EntityStore::createWithId(EntityID id, std::string name)
    {
        Entity e;
        e.id = id;
        e.name = name;

        auto handle = m_pool.emplace(std::move(e));

        m_byId[id] = handle;
        return handle;
    }

    Entity *EntityStore::get(EntityHandle h)
    {
        return m_pool.get(h);
    }

    std::optional<EntityHandle> EntityStore::find(EntityID id) const
    {
        auto it = m_byId.find(id);

        if (it == m_byId.end())
        {
            return std::nullopt;
        }

        return it->second;
    }

    void EntityStore::destroy(EntityHandle h)
    {

        auto entity = get(h);

        if (!entity)
        {
            return;
        }
        m_byId.erase(entity->id);
        m_pool.free(h);
    };

    void EntityStore::reclaimSlot(EntityHandle h)
    {
        m_pool.reclaim(h);
    }
    void EntityStore::deactivateSlot(EntityHandle h)
    {
        if (auto *e = get(h))
            m_byId.erase(e->id);
        m_pool.deactivate(h);
    }
    bool EntityStore::reactivateSlot(EntityHandle h, Entity e)
    {
        auto id = e.id;
        bool ok = m_pool.reactivate(h, std::move(e));
        if (ok)
            m_byId[id] = h;
        return ok;
    }

    void EntityStore::forEach(std::function<void(const Entity &entity)> callback)
    {
        m_pool.forEach(
            [&](const EntityHandle &handle, const Entity &entity)
            {
                callback(entity);
            });
    }

    void EntityStore::clear()
    {
        m_pool.clear();
        m_byId.clear();
    }

} // namespace nitro::renderer
