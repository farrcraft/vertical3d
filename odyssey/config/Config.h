/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <unordered_map>
#include <string>
#include <string_view>

#include "BindingContext.h"
#include "../../v3dlibs/asset/Json.h"
#include "../../v3dlibs/asset/Manager.h"

#include <boost/shared_ptr.hpp>

namespace odyssey::config {
    /**
     * JSON file-derived configuration
     **/
    class Config final {
     public:
        /**
         **/
        Config(const boost::shared_ptr<v3d::core::Logger> &logger);

        /**
         * Load all of the standard config
         * 
         * @param assetManager loads JSON files as managed assets
         * 
         * @return true if all of the config is successfully loaded
         **/
        bool load(const boost::shared_ptr<v3d::asset::Manager>& assetManager);

        /**
         **/
        int windowWidth() const;

        /**
         **/
        int windowHeight() const;

     protected:
        /**
         * Load the bindings.json config
         * 
         * @param bindings the json file asset to load bindings from
         * 
         * @return true if bindings are successfully loaded
         **/
        bool loadBindings(const boost::shared_ptr<v3d::asset::Json>& bindings);

        /**
         * Load the root config file
         * 
         * * @param config the json file asset to load config from
         * 
         * @return true if the config is successfully loaded
         **/
        bool loadConfig(const boost::shared_ptr<v3d::asset::Json>& config);


     private:
        std::unordered_map<std::string_view, boost::shared_ptr<BindingContext> > contexts_;
        boost::shared_ptr<v3d::core::Logger> logger_;
        std::string configPath_;
        int windowWidth_;
        int windowHeight_;
    };
};  // namespace odyssey::config

