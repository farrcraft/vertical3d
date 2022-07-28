/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <unordered_map>
#include <string>
#include <string_view>

#include "Type.h"
#include "BindingContext.h"
#include "../asset/Json.h"
#include "../asset/Manager.h"

#include <boost/shared_ptr.hpp>

namespace v3d::config {
    /**
     * JSON file-derived configuration
     **/
    class Config final {
     public:
        /**
         **/
        Config(const boost::shared_ptr<v3d::log::Logger> &logger);

        /**
         * Load all of the standard config
         * 
         * @param assetManager loads JSON files as managed assets
         * 
         * @return true if all of the config is successfully loaded
         **/
        bool load(const boost::shared_ptr<v3d::asset::Manager>& assetManager);

        /**
         * Get a loaded config
         **/
        boost::shared_ptr<v3d::asset::Json> get(Type configType);

     protected:
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
        boost::shared_ptr<v3d::log::Logger> logger_;
        std::unordered_map<Type, boost::shared_ptr<v3d::asset::Json> > configs_;
    };
};  // namespace v3d::config

