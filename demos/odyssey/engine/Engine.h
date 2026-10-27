/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/asset/Worker.h>
#include <api/engine/Engine.h>
#include <api/event/Event.h>
#include <api/event/kind/MouseMotion.h>
#include <api/grid/TileCoord.h>
#include <api/input/Devices.h>
#include <api/log/Logger.h>
#include <demos/odyssey/render/Renderer.h>
#include <demos/odyssey/system/Movement.h>
#include <demos/odyssey/tile/World.h>

#include <string>

#include "Player.h"

#include <glm/vec2.hpp>

namespace odyssey::engine {

/**
 * The odyssey application. It registers the game's commands, loads the world, and moves the
 * player each simulation step. The api's game loop calls it.
 **/
class Engine final : public v3d::engine::Engine {
 public:
    /**
     * Constructor.
     * 
     * @param appPath The fully qualified base path name from which all relative 
     *                paths will be derived.
     **/
    explicit Engine(const std::string& appPath);

    /**
     * Called once a frame. Polls the worker, which hands each finished region load to the world.
     * Then requests the regions the player is approaching and releases the ones it has left.
     **/
    bool tick(unsigned int delta) override;

    /**
     * Advance the game world by one simulation step
     * @return bool
     **/
    bool simulate(float step) override;

    /**
     * Draw the current frame
     * @return bool
     **/
    bool render() override;

 protected:
    /**
     * Load the world, place the player on its start tile, and build the renderer.
     *
     * @return false when the world cannot be loaded
     **/
    bool start() override;

    /**
     * @return bool
     **/
    bool release() override;

 private:
    /**
     * Track the cursor. A click carries no position of its own, so the last motion is
     * where the click happened.
     **/
    void handleMotion(const v3d::event::kind::MouseMotion& event);

    /**
     * Route the player to the tile under the cursor, replacing whatever it was walking. A click
     * on a wall, in another region, or somewhere no route reaches leaves it where it is.
     **/
    void walkToCursor();

    /**
     * Move the player one tile in a direction, if World::passable() allows that tile. A step can
     * therefore cross into the next region once that region is loaded. While the player walks a
     * route, a step stops the route and does not move.
     **/
    void step(int dx, int dy);

    /**
     * @return the world tile the player is standing on
     **/
    v3d::grid::TileCoord playerTile() const;

    boost::shared_ptr<Player> player_;
    // declared before world_, so world_ is destroyed first. The worker's destructor then drops
    // every completion that has not run
    boost::shared_ptr<v3d::asset::Worker> worker_;
    boost::shared_ptr<odyssey::tile::World> world_;
    glm::vec2 cursor_{0.0F, 0.0F};
    boost::shared_ptr<odyssey::render::Renderer> renderer_;
    boost::shared_ptr<odyssey::system::Movement> movementSystem_;
    // disconnects this object's delegate from the dispatcher when this object is destroyed
    entt::scoped_connection motion_;
};

};  // namespace odyssey::engine
