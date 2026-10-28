/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Player.h"

#include <api/grid/TileCoord.h>

namespace odyssey::engine {

/**
 **/
Player::Player(entt::registry* registry) {
    // register a player entity
    id_ = registry->create();
    // create the components attached to player entity
    registry->emplace<v3d::grid::TileCoord>(id_, 0, 0);
}

/**
 **/
entt::entity Player::entity() const noexcept {
    return id_;
}

};  // namespace odyssey::engine
