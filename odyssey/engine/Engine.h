/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../../api/log/Logger.h"
#include "Player.h"

#include "../../api/engine/Engine.h"
#include "../../api/asset/Manager.h"
#include "../../api/event/Event.h"
#include "../../api/config/Config.h"
#include "../../api/input/Engine.h"
#include "../render/Renderer.h"
#include "../system/Movement.h"

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
     * Advance the game world time
     * @return bool
     **/
    bool tick(unsigned int delta) override;

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

    boost::shared_ptr<Player> player_;
    boost::shared_ptr<odyssey::render::Renderer> renderer_;
    boost::shared_ptr<odyssey::system::Movement> movementSystem_;
};

};  // namespace odyssey::engine
