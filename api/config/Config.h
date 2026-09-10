/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/asset/kind/Json.h>

#include <unordered_map>
#include <string>
#include <string_view>

#include "Type.h"

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
    boost::shared_ptr<v3d::asset::kind::Json> get(Type configType);

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::unordered_map<Type, boost::shared_ptr<v3d::asset::kind::Json> > configs_;
};
};  // namespace v3d::config

