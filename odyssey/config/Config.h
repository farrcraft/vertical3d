/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "BindingContext.h"
#include "../asset/Manager.h"

#include <boost/shared_ptr.hpp>

#include <unordered_map>
#include <string_view>

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
        bool load(const boost::shared_ptr<odyssey::asset::Manager>& assetManager);
        
    protected:
        /**
         * Load the bindings.json config
         * 
         * @param bindings the json file asset to load bindings from
         * 
         * @return true if bindings are successfully loaded
         **/
        bool loadBindings(const boost::shared_ptr<odyssey::asset::Json>& bindings);

    private:
        std::unordered_map<std::string_view, boost::shared_ptr<BindingContext> > contexts_;
        boost::shared_ptr<v3d::core::Logger> logger_;
    };
};
