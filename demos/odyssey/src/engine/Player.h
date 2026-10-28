/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <entt/entt.hpp>

namespace odyssey::engine {
/**
 * The player, as an entity in the game's registry with a world tile.
 **/
class Player final {
 public:
    /**
     * Create the player's entity, on the tile at the world's origin.
     **/
    explicit Player(entt::registry *registry);

    /**
     * The entity that holds the player's components.
     **/
    entt::entity entity() const noexcept;

 private:
    entt::entity id_;
};
};  // namespace odyssey::engine
