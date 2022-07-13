/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Player.h"
#include "../component/Position.h"

using namespace odyssey::engine;

/**
 **/
Player::Player(entt::registry &registry) {
	// register a player entity
	id_ = registry.create();
	// create the components attached to player entity
	registry.emplace<odyssey::component::Position>(id_, 0, 0);
}
