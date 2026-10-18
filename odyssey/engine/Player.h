/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Sprite.h"

#include <entt/entt.hpp>

namespace odyssey::engine {
/**
 * 
 **/
class Player final {
 public:
    /**
     **/
    Player(entt::registry *registry);

    /**
     * The entity that holds the player's components.
     **/
    entt::entity entity() const noexcept;

 private:
    entt::entity id_;
    Sprite sprite_;
};
};  // namespace odyssey::engine
