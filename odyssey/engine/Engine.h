/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/config/Config.h>
#include <api/engine/Engine.h>
#include <api/event/Event.h>
#include <api/event/MouseMotion.h>
#include <api/input/Engine.h>
#include <api/log/Logger.h>
#include <odyssey/render/Renderer.h>
#include <odyssey/system/Movement.h>
#include <odyssey/tile/Map.h>

#include <string>

#include "Player.h"

#include <glm/vec2.hpp>

namespace odyssey::engine {

/**
 * This is the game engine.
 * It is responsible for the main game loop
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
     * Initialize the engine.
     * Initialization includes only the minimal amount of work required to get
     * a window displayed on the screen.
     * 
     * @return bool
     **/
    bool initialize();

    /**
     * Advance the game world by one fixed simulation step
     * @return bool
     **/
    bool simulate(float step) override;

    /**
     * Draw the current frame
     * @return bool
     **/
    bool render() override;

    /**
     * @return bool
     **/
    bool shutdown() override;

 private:
    /**
     * Handle a mapped event, one of the destinations named in data/mappings.json.
     **/
    void handleEvent(const v3d::event::Event& event);

    /**
     * Track the cursor. A click carries no position of its own, so the last motion is
     * where the click happened - which is how the editor reads a pick too.
     **/
    void handleMotion(const v3d::event::MouseMotion& event);

    /**
     * Route the player to the tile under the cursor, replacing whatever it was walking.
     * A click on a wall, off the map, or somewhere no route reaches leaves it where it is.
     **/
    void walkToCursor();

    /**
     * One tile in a direction, if the player is not already walking a route and the tile
     * can be entered. The grid answers that, so a step and a route refuse the same tiles.
     **/
    void step(int dx, int dy);

    /**
     * @return the tile the player is standing on
     **/
    v3d::grid::TileCoord playerTile() const;

    boost::shared_ptr<Player> player_;
    boost::shared_ptr<odyssey::tile::Map> map_;
    glm::vec2 cursor_{0.0f, 0.0f};
    boost::shared_ptr<odyssey::render::Renderer> renderer_;
    boost::shared_ptr<odyssey::system::Movement> movementSystem_;
};

};  // namespace odyssey::engine
