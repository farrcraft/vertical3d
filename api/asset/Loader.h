/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/
#pragma once

#include <api/log/Logger.h>

#include <string_view>
#include <typeindex>
#include <typeinfo>

#include "Asset.h"

#include <boost/shared_ptr.hpp>

namespace v3d::asset {

/**
 * The base interface for asset loaders.
 *
 * A loader is registered for the class of asset it produces, its kind. Every asset it returns
 * is of that class.
 *
 * A loader keeps no state between calls. One loader serves every caller of the same manager,
 * so a setting left on it by one caller would affect the next. A load reads everything it
 * needs from the file.
 **/
class Loader {
 public:
    /**
     * @return the class of asset this loader produces
     **/
    std::type_index kind() const noexcept;

    /**
     * Load an asset
     *
     * @param name The path of the asset to be loaded
     * @return the asset, or null with a line in the log saying why. Every loader reports a
     *         failure that way and none throws
     **/
    virtual boost::shared_ptr<Asset> load(std::string_view name) = 0;

 protected:
    /**
     * @param kind the class of asset this loader produces, as typeid(kind::Json)
     * @param logger the log a failed load is reported to
     **/
    Loader(const std::type_info& kind, const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * @return the log a loader reports a failed load to
     **/
    const boost::shared_ptr<v3d::log::Logger>& logger() const noexcept;

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::type_index kind_;
};
};  // namespace v3d::asset
