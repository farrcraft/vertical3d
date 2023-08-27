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
#include "../../api/config/Config.h"
#include "../../api/input/Engine.h"
#include "../../api/render/realtime/2D/Engine2D.h"
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
        bool tick();

        /**
         * @return bool
         **/
        bool shutdown();

     private:
        boost::shared_ptr<Player> player_;
        boost::shared_ptr<v3d::render::realtime::Engine2D> renderEngine_;
        boost::shared_ptr<odyssey::system::Movement> movementSystem_;
    };

};  // namespace odyssey::engine
