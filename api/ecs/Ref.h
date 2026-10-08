/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <entt/entt.hpp>

namespace v3d::ecs {

/**
 * An entity held across frames that reads as null once the entity is destroyed.
 *
 * EnTT reuses the index of a destroyed entity and bumps its version. A plain entt::entity kept
 * past a destroy can therefore come to name a different entity, created later at the same index.
 * A Ref compares the version it stored with the registry's, so it reads null instead.
 *
 * It is for view state: a selection, an inspector's target, the entity a camera follows. A
 * container the rules keep, such as a turn order, is kept correct by whatever destroys an entity.
 * Checking on read there would hide a fault in that upkeep, so such a container holds plain
 * entities.
 **/
class Ref final {
 public:
    Ref() noexcept = default;

    /**
     * @param entity the entity to hold, or entt::null for none
     **/
    explicit Ref(entt::entity entity) noexcept :
        entity_(entity) {
    }

    /**
     * @return the entity held, or entt::null when none is held or the one held has been
     *         destroyed, even if another entity has since been created at its index
     **/
    entt::entity get(const entt::registry& registry) const noexcept {
        return registry.valid(entity_) ? entity_ : entt::null;
    }

    /**
     * @param entity the entity to hold from now on, or entt::null for none
     **/
    void set(entt::entity entity) noexcept {
        entity_ = entity;
    }

    /**
     * Hold no entity.
     **/
    void clear() noexcept {
        entity_ = entt::null;
    }

 private:
    entt::entity entity_ = entt::null;
};

};  // namespace v3d::ecs
