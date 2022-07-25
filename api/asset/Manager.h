/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <unordered_map>

#include "Asset.h"
#include "Loader.h"
#include "Type.h"

#include "../log/Logger.h"

#include <boost/filesystem.hpp>

namespace v3d::asset {
    /**
     * The Asset Manager provides an access point for mapping and loading assets
     * within a single path.
     **/
    class Manager final {
     public:
        /**
         **/
        Manager(std::string_view path, const boost::shared_ptr<v3d::log::Logger>& logger);

        /**
         * Load an asset
         * 
         * @param name
         * @param type
         **/
        boost::shared_ptr<Asset> load(std::string_view name, asset::Type t);

        /**
         * Load an asset, guessing the type from its filename extension.
         *
         * @param name
         **/
        boost::shared_ptr<Asset> loadTypeFromExt(std::string_view name);

     protected:
        /**
         **/
        boost::shared_ptr<Loader> resolveLoader(asset::Type t);

     private:
        boost::filesystem::path path_;
        std::unordered_map<asset::Type, boost::shared_ptr<Loader>> loaders_;
        boost::shared_ptr<v3d::log::Logger> logger_;
    };

};  // namespace v3d::asset
