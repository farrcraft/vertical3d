/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Engine.h"

#include <api/asset/Worker.h>
#include <api/engine/Engine.h>
#include <api/event/Event.h>
#include <api/event/State.h>
#include <api/event/kind/MouseMotion.h>
#include <api/grid/Pathfinding.h>
#include <api/grid/TileCoord.h>
#include <api/ui/shell/GameMenu.h>
#include <demos/odyssey/src/render/Renderer.h>
#include <demos/odyssey/src/system/Movement.h>
#include <demos/odyssey/src/tile/World.h>

#include "Path.h"
#include "Player.h"
#include "Unit.h"

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

namespace odyssey::engine {

/**
 **/
Engine::Engine(const std::string& appPath) :
    v3d::engine::Engine(appPath),
    shell_(this, { .menu = true, .statistics = "odyssey::toggleStatistics" }) {
    // the bindings name no state, so a command receives both the press and the release of its
    // key. Only the press moves the player, or every key would take two steps. A held key's
    // repeats are not releases, so each repeat takes a step and holding a key walks. Nothing
    // moves the player while the menu is up
    auto press = [this](const std::string& name, const std::function<void()>& action) {
        commands().add(name, [this, action](const v3d::event::Event& event) {
            if (!paused_ && event.state() != v3d::event::State::Released) {
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

    const v3d::ui::shell::GameMenu::Suspend pause = [this](bool suspended) { paused_ = suspended; };
    if (!shell_.start(pause)) {
        return false;
    }

    worker_ = boost::make_shared<v3d::asset::Worker>();
    world_ = boost::make_shared<odyssey::tile::World>(logger(), assets(), worker_);
    // the world document is a kind of odyssey's own, and config.json lists it beside the others
    if (!world_->load(manifest()->get(odyssey::tile::World::DOCUMENT))) {
        // the world holds the board and its collision rules, so the game cannot run without
        // it. load() has already logged what it could not read
        return false;
    }

    player_ = boost::make_shared<Player>(&registry());
    const v3d::grid::TileCoord start = world_->start();
    registry().replace<v3d::grid::TileCoord>(player_->entity(), start.x, start.y);
    registry().emplace<odyssey::engine::Path>(player_->entity());

    movementSystem_ = boost::make_shared<odyssey::system::Movement>(&registry());

    renderer_ = boost::make_shared<odyssey::render::Renderer>(window(), logger(), assets(), &registry(), &shell_);
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
    return registry().get<v3d::grid::TileCoord>(player_->entity());
}

/**
 **/
void Engine::walkToCursor() {
    const v3d::grid::TileCoord from = playerTile();
    const v3d::grid::TileCoord view = world_->view(from, v3d::grid::TileCoord{unit::screen_tile_width, unit::screen_tile_height});
    // the cursor's coordinates are never negative, so integer division gives its tile
    const v3d::grid::TileCoord goal{
        view.x + (static_cast<int>(cursor_.x) / unit::tile_width),
        view.y + (static_cast<int>(cursor_.y) / unit::tile_height)};
    // a route crosses every loaded region, and stops at one that is not loaded
    if (!world_->passable(goal)) {
        return;
    }
    std::vector<v3d::grid::TileCoord> route = v3d::grid::findPath(world_->grids(), from, goal);
    odyssey::engine::Path& path = registry().get<odyssey::engine::Path>(player_->entity());
    path.tiles = std::move(route);
    // the first tile findPath returns is the one the player stands on, so the next is index 1
    path.next = 1;
    path.elapsed = 0.0F;
}

/**
 **/
void Engine::step(int dx, int dy) {
    odyssey::engine::Path& path = registry().get<odyssey::engine::Path>(player_->entity());
    if (!path.tiles.empty()) {
        // a key stops a walk in progress, and does not also take a step
        path.tiles.clear();
        path.next = 0;
        path.elapsed = 0.0F;
        return;
    }
    const v3d::grid::TileCoord from = playerTile();
    const v3d::grid::TileCoord to{from.x + dx, from.y + dy};
    if (!world_->passable(to)) {
        return;
    }
    registry().replace<v3d::grid::TileCoord>(player_->entity(), to.x, to.y);
}

/**
 **/
bool Engine::release() {
    // the screen draws through the renderer, so it goes first
    shell_.release();
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
    if (paused_) {
        return true;
    }
    if (!movementSystem_->simulate(step)) {
        return false;
    }
    // look after movement, so the next frame draws what the player sees from its new tile
    world_->look(playerTile());
    return true;
}

};  // namespace odyssey::engine
