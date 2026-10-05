/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Asset.h"
#include "Loader.h"
#include "Type.h"

#include <boost/filesystem.hpp>

namespace v3d::asset {
/**
 * The Asset Manager provides an access point for mapping and loading assets
 * within a single path.
 *
 * It loads what a loader has been registered for. A manager starts with the two that read a
 * document, Json and Text; the loaders for pictures, models and typefaces are in
 * api/asset/media and the one for sound in api/audio, and each registers its own - ADR-0079.
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
     * Load through a loader for the asset's type, and resolve a file to that type by any of
     * the extensions given. Registering a type a second time replaces the loader.
     *
     * @param extensions with the dot and in lower case, as ".png"; a file's own extension is
     *        lowered before it is looked up
     **/
    void registerLoader(const boost::shared_ptr<Loader>& loader, const std::vector<std::string>& extensions);

    /**
     * Load an asset, guessing the type from its filename extension.
     *
     * @param name
     **/
    boost::shared_ptr<Asset> loadTypeFromExt(std::string_view name);

    /**
     * Get the loader for an asset type
     * 
     * @param type
     **/
    boost::shared_ptr<Loader> resolveLoader(asset::Type t);

 private:
    boost::filesystem::path path_;
    std::unordered_map<asset::Type, boost::shared_ptr<Loader>> loaders_;
    std::unordered_map<std::string, asset::Type> extensions_;
    boost::shared_ptr<v3d::log::Logger> logger_;
};

};  // namespace v3d::asset
