/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../../api/log/Logger.h"
#include "Player.h"

#include "../../api/asset/Manager.h"
#include "../config/Config.h"
#include "../../api/input/Engine.h"
#include "../../api/ui/Window.h"
#include "../render/Engine.h"
#include "../system/Movement.h"

namespace odyssey::engine {

    /**
     * This is the game engine.
     * It is responsible for the main game loop
     **/
    class Engine final {
     public:
        /**
         * Constructor.
         * 
         * @param appPath The fully qualified base path name from which all relative 
         *                paths will be derived.
         **/
        explicit Engine(const std::string_view& appPath);

        /**
         * Initialize the engine.
         * Initialization includes only the minimal amount of work required to get
         * a window displayed on the screen.
         * 
         * @return bool
         **/
        bool initialize();

        /**
         * The game loop entry point
         * @return bool
         **/
        bool run();

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
        std::string appPath_;
        boost::shared_ptr<odyssey::config::Config> config_;
        boost::shared_ptr<v3d::ui::Window> window_;
        boost::shared_ptr<v3d::log::Logger> logger_;

        boost::shared_ptr<Player> player_;

        boost::shared_ptr<odyssey::render::Engine> renderEngine_;
        boost::shared_ptr<v3d::input::Engine> inputEngine_;

        boost::shared_ptr<v3d::asset::Manager> assetManager_;

        entt::registry registry_;
        boost::shared_ptr<entt::dispatcher> dispatcher_;

        boost::shared_ptr<odyssey::system::Movement> movementSystem_;
    };

};  // namespace odyssey::engine
