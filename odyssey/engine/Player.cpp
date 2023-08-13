/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Player.h"
#include "../../api/ecs/component/PositionFixed2D.h"

namespace odyssey::engine {

    /**
     **/
    Player::Player(entt::registry& registry) {
        // register a player entity
        id_ = registry.create();
        // create the components attached to player entity
        registry.emplace<v3d::ecs::component::PositionFixed2D>(id_, 0, 0);
    }

};  // namespace odyssey::engine
