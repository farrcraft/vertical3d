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
 * The JSON documents an app's config.json lists, each stored under the type its entry names.
 **/
class Config final {
 public:
    /**
     **/
    Config(const boost::shared_ptr<v3d::log::Logger> &logger);

    /**
     * Read config.json, then load every file it lists.
     * 
     * @param assetManager loads JSON files as managed assets
     * 
     * An entry with an empty type is refused. typeName() returns the empty string for
     * Type::Unknown, so get(Type::Unknown) would otherwise find that entry.
     *
     * @return true if every listed file loaded. On false, the documents stored by an earlier
     *         load stay in place.
     **/
    bool load(const boost::shared_ptr<v3d::asset::Manager>& assetManager);

    /**
     * Get a loaded config of a type the api reads.
     **/
    boost::shared_ptr<v3d::asset::kind::Json> get(Type configType);

    /**
     * Get a loaded config by the type its entry names. The type may be one of the app's own,
     * because load() stores a type the api does not read like any other.
     *
     * @return the document, or null when config.json names none of that type
     **/
    boost::shared_ptr<v3d::asset::kind::Json> get(std::string_view type);

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::unordered_map<std::string, boost::shared_ptr<v3d::asset::kind::Json> > configs_;
};
};  // namespace v3d::config

