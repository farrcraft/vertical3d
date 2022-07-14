/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../../v3dlibs/core/Logger.h"

#include <string_view>

namespace odyssey::config {

    /**
     * Bootstrap configuration
     * This is a minimal configuration file that provides the core settings
     * necessary for booting up the engine.
     **/
    class Bootstrap {
     public:
        /**
         **/
        Bootstrap();

        /**
         * @param Logger& logger
         * @return bool
         **/
        bool load(const boost::shared_ptr<v3d::core::Logger>& logger);

        /**
         **/
        int windowWidth() const;

        /**
         **/
        int windowHeight() const;

        /**
         **/
        std::string_view dataPath() const;

     private:
        std::string dataPath_;
        int windowWidth_;
        int windowHeight_;
    };

};  // namespace odyssey::config
