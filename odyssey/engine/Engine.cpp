/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Engine.h"

#include <api/grid/TileCoord.h>
#include <api/engine/Feature.h>
#include <api/grid/Pathfinding.h>
#include <api/grid/TileGrid.h>

#include "Path.h"
#include "Unit.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

namespace odyssey::engine {

/**
 **/
Engine::Engine(const std::string& appPath) :
    v3d::engine::Engine(appPath) {
    // the bindings name no state, so both edges arrive and only the press is a move - a
    // release would otherwise take a second step off every key. A held key's repeats each
    // take a step, so holding a key walks
    auto press = [this](const std::string& name, const std::function<void()>& action) {
        commands().add(name, [action](const v3d::event::Event& event) {
            if (event.state() != v3d::event::State::Released) {
                action();
            }
        });
    };
    press("odyssey::use", [this]() { walkToCursor(); });
    press("odyssey::moveForward", [this]() { step(0, -1); });
    press("odyssey::moveBackward", [this]() { step(0, 1); });
    press("odyssey::moveLeft", [this]() { step(-1, 0); });
    press("odyssey::moveRight", [this]() { step(1, 0); });
}

/**
 **/
bool Engine::start() {
    window()->caption("Odyssey");

    worker_ = boost::make_shared<v3d::asset::Worker>();
    world_ = boost::make_shared<odyssey::tile::World>(logger(), assets(), worker_);
    // the world is a document of the app's own, which config.json names like the rest
    if (!world_->load(config()->get("world"))) {
        // the world is the board and the collision rules both, so there is no sensible game
        // without one - the loader has already said what it could not read
        return false;
    }

    player_ = boost::make_shared<Player>(&registry_);
    const v3d::grid::TileCoord start = world_->start();
    registry_.replace<v3d::grid::TileCoord>(player_->entity(), start.x, start.y);
    registry_.emplace<odyssey::engine::Path>(player_->entity());

    movementSystem_ = boost::make_shared<odyssey::system::Movement>(&registry_);

    renderer_ = boost::make_shared<odyssey::render::Renderer>(window(), logger(), assets(), &registry_);
    renderer_->player(player_);
    renderer_->world(world_);

    motion_ = dispatcher()->sink<v3d::event::kind::MouseMotion>().connect<&Engine::handleMotion>(*this);

    return true;
}

/**
 **/
void Engine::handleMotion(const v3d::event::kind::MouseMotion& event) {
    cursor_ = event.position();
}

/**
 **/
v3d::grid::TileCoord Engine::playerTile() const {
    return registry_.get<v3d::grid::TileCoord>(player_->entity());
}

/**
 **/
void Engine::walkToCursor() {
    const v3d::grid::TileCoord from = playerTile();
    const v3d::grid::TileCoord view = world_->view(from, v3d::grid::TileCoord{unit::screen_tile_width, unit::screen_tile_height});
    // a cursor is never left of or above the window, so a pixel divides down to its tile
    const v3d::grid::TileCoord goal{
        view.x + static_cast<int>(cursor_.x) / unit::tile_width,
        view.y + static_cast<int>(cursor_.y) / unit::tile_height};
    // a route stays within the region the player stands in
    const std::optional<std::size_t> region = world_->regionAt(from);
    if (!region || world_->regionAt(goal) != region || !world_->passable(goal)) {
        return;
    }
    const v3d::grid::TileGrid& grid = *world_->map(*region)->grid();
    std::vector<v3d::grid::TileCoord> route = v3d::grid::findPath(grid, world_->local(*region, from), world_->local(*region, goal));
    for (v3d::grid::TileCoord& tile : route) {
        tile = world_->world(*region, tile);
    }
    odyssey::engine::Path& path = registry_.get<odyssey::engine::Path>(player_->entity());
    path.tiles = std::move(route);
    // findPath returns the tile the mover is standing on first, so the walk starts at 1
    path.next = 1;
    path.elapsed = 0.0f;
}

/**
 **/
void Engine::step(int dx, int dy) {
    odyssey::engine::Path& path = registry_.get<odyssey::engine::Path>(player_->entity());
    if (!path.tiles.empty()) {
        // a key interrupts a walk rather than fighting it for the next tile
        path.tiles.clear();
        path.next = 0;
        path.elapsed = 0.0f;
        return;
    }
    const v3d::grid::TileCoord from = playerTile();
    const v3d::grid::TileCoord to{from.x + dx, from.y + dy};
    if (!world_->passable(to)) {
        return;
    }
    registry_.replace<v3d::grid::TileCoord>(player_->entity(), to.x, to.y);
}

/**
 **/
bool Engine::release() {
    if (renderer_) {
        // the device has to be idle before the window it presents to is destroyed
        renderer_->shutdown();
    }
    return true;
}

/**
 **/
bool Engine::render() {
    if (!v3d::engine::Engine::render()) {
        return false;
    }
    renderer_->draw();
    return true;
}

/**
 **/
bool Engine::tick(unsigned int delta) {
    if (!v3d::engine::Engine::tick(delta)) {
        return false;
    }
    worker_->poll();
    world_->stream(playerTile());
    return true;
}

/**
 **/
bool Engine::simulate(float step) {
    if (!v3d::engine::Engine::simulate(step)) {
        return false;
    }
    // Step various systems, e.g. Movement System, Collision System, Combat System, etc
    if (!movementSystem_->simulate(step)) {
        return false;
    }
    // after the systems have moved anything, and before the frame that draws what the
    // player can see from where it now stands
    world_->look(playerTile());
    return true;
}

};  // namespace odyssey::engine
