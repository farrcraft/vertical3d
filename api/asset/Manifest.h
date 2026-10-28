/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/kind/Json.h>
#include <api/log/Logger.h>

#include <string>
#include <string_view>
#include <unordered_map>

#include "Manager.h"

#include <boost/shared_ptr.hpp>

namespace v3d::asset {
/**
 * The JSON documents an app lists in one file, each held under the kind its entry names.
 *
 * The list is {"configs": [{"type": "window", "file": "window.json"}, ...]}. A kind is any
 * name. The library that reads a kind of document declares its name, and an app names kinds of
 * its own the same way.
 **/
class Manifest final {
 public:
    /**
     **/
    explicit Manifest(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read the list, then load every file it names.
     *
     * @param assets loads the list and each file it names, by the manager's path
     * @param list the name of the list, as "config.json"
     *
     * An entry with an empty kind is refused.
     *
     * @return true if every listed file loaded. On false, the documents held from an earlier
     *         load stay in place.
     **/
    bool load(Manager& assets, std::string_view list);

    /**
     * @return the document the list names under a kind, or null when it names none
     **/
    boost::shared_ptr<kind::Json> get(std::string_view kind) const;

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::unordered_map<std::string, boost::shared_ptr<kind::Json> > documents_;
};
};  // namespace v3d::asset
