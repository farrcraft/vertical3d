/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Engine.h"

#include <api/ecs/component/PositionFixed2D.h>
#include <api/engine/Feature.h>
#include <api/grid/Pathfinding.h>

#include "Path.h"
#include "Unit.h"

#include <string>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

namespace odyssey::engine {

namespace {
/**
 * The board, read through the asset manager like any other file the app names.
 **/
const char* const mapName = "map.json";

};  // namespace

/**
 **/
Engine::Engine(const std::string& appPath) :
    v3d::engine::Engine(appPath) {
}

/**
 **/
bool Engine::initialize() {
    if (!v3d::engine::Engine::initialize(static_cast<int>(
        v3d::engine::Feature::Config |
        v3d::engine::Feature::Window |
        v3d::engine::Feature::MouseInput |
        v3d::engine::Feature::KeyboardInput))) {
        return false;
    }

    window_->caption("Odyssey");

    map_ = boost::make_shared<odyssey::tile::Map>(logger_);
    if (!map_->load(boost::dynamic_pointer_cast<v3d::asset::kind::Json>(
            assetManager_->loadTypeFromExt(mapName)))) {
        // the map is the board and the collision rules both, so there is no sensible game
        // without one - the loader has already said what it could not read
        return false;
    }

    player_ = boost::make_shared<Player>(&registry_);
    const v3d::grid::TileCoord start = map_->start();
    registry_.replace<v3d::ecs::component::PositionFixed2D>(player_->entity(), start.x, start.y);
    registry_.emplace<odyssey::engine::Path>(player_->entity());

    movementSystem_ = boost::make_shared<odyssey::system::Movement>(&registry_);

    sight_ = boost::make_shared<odyssey::tile::Sight>();
    sight_->look(*map_->grid(), start);

    renderer_ = boost::make_shared<odyssey::render::Renderer>(window(), logger_, assetManager_, &registry_);
    renderer_->player(player_);
    renderer_->map(map_);
    renderer_->sight(sight_);

    // one sink for every mapped event: a device event is resolved to an action by the
    // bindings before it gets here, so nothing subscribes to a key
    dispatcher_->sink<v3d::event::Event>().connect<&Engine::handleEvent>(*this);
    dispatcher_->sink<v3d::event::kind::MouseMotion>().connect<&Engine::handleMotion>(*this);

    return true;
}

/**
 **/
void Engine::handleEvent(const v3d::event::Event& event) {
    if (event.context()->name() != "odyssey") {
        return;
    }
    if (event.name() == "quit") {
        // not shutdown() - the event loop ticks and renders once more after a handler
        // returns, and that frame would be drawn into a destroyed window
        quit();
        return;
    }
    // the movement bindings name no state, so both edges arrive here and only the press
    // is a move - a release would otherwise take a second step off every key
    if (event.state() == v3d::event::State::Released) {
        return;
    }
    if (event.name() == "use") {
        walkToCursor();
    } else if (event.name() == "moveForward") {
        step(0, -1);
    } else if (event.name() == "moveBackward") {
        step(0, 1);
    } else if (event.name() == "moveLeft") {
        step(-1, 0);
    } else if (event.name() == "moveRight") {
        step(1, 0);
    }
}

/**
 **/
void Engine::handleMotion(const v3d::event::kind::MouseMotion& event) {
    cursor_ = event.position();
}

/**
 **/
v3d::grid::TileCoord Engine::playerTile() const {
    const v3d::ecs::component::PositionFixed2D& position =
        registry_.get<v3d::ecs::component::PositionFixed2D>(player_->entity());
    return v3d::grid::TileCoord{position.x(), position.y()};
}

/**
 **/
void Engine::walkToCursor() {
    const v3d::grid::TileCoord goal{
        static_cast<int>(cursor_.x) / unit::tile_width,
        static_cast<int>(cursor_.y) / unit::tile_height};
    if (!map_->grid()->passable(goal)) {
        return;
    }
    odyssey::engine::Path& path = registry_.get<odyssey::engine::Path>(player_->entity());
    path.tiles = v3d::grid::findPath(*map_->grid(), playerTile(), goal);
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
    if (!map_->grid()->passable(to)) {
        return;
    }
    registry_.replace<v3d::ecs::component::PositionFixed2D>(player_->entity(), to.x, to.y);
}

/**
 **/
bool Engine::shutdown() {
    if (renderer_) {
        // the device has to be idle before the window it presents to is destroyed
        renderer_->shutdown();
    }
    if (!v3d::engine::Engine::shutdown()) {
        return false;
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
    sight_->look(*map_->grid(), playerTile());
    return true;
}

};  // namespace odyssey::engine
