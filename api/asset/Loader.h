/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/
#pragma once

#include <api/log/Logger.h>

#include <string_view>

#include "Asset.h"
#include "Type.h"

#include <boost/shared_ptr.hpp>

namespace v3d::asset {

/**
 * The base interface for asset loaders.
 *
 * A loader holds nothing between calls: one is shared by everything that loads through the
 * same manager, so a setting left on it by one caller would be found by the next. Whatever a
 * load needs it reads from the file.
 **/
class Loader {
 public:
    /**
     * Get the asset type
     * 
     * @return Type
     **/
    Type type() const;

    /**
     * Load an asset
     *
     * @param name The path of the asset to be loaded
     * @return the asset, or null with a line in the log saying why - every loader answers a
     *         failure that way and none throws
     **/
    virtual boost::shared_ptr<Asset> load(std::string_view name) = 0;

 protected:
    /**
     * Default constructor
     *
     * @param Type t The asset type this loader provides
     **/
    Loader(Type t, const boost::shared_ptr<v3d::log::Logger>& logger);

    boost::shared_ptr<v3d::log::Logger> logger_;

 private:
    Type type_;
};
};  // namespace v3d::asset
