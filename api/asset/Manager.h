/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <string>
#include <string_view>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "Asset.h"
#include "Loader.h"
#include "Type.h"

#include <boost/filesystem.hpp>
#include <boost/pointer_cast.hpp>

namespace v3d::asset {
/**
 * The Asset Manager provides an access point for mapping and loading assets
 * within a single path.
 *
 * It loads what a loader has been registered for. A manager starts with the two that read a
 * document, Json and Text; the loaders for pictures and models are in api/asset/media and the
 * one for sound in api/audio, and each library registers its own.
 *
 * **Every way of loading reports a failure the same way: no asset, and a line in the log.**
 * That covers a missing file, one that does not decode, a type with no registered loader and
 * an unregistered extension, so a caller checks one pointer whatever went wrong.
 **/
class Manager final {
 public:
    /**
     **/
    Manager(std::string_view path, const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Load an asset through the loader registered for its type.
     *
     * @return the asset, or null
     **/
    boost::shared_ptr<Asset> load(std::string_view name, asset::Type t);

    /**
     * Load an asset, the type found from its extension.
     *
     * @return the asset, or null
     **/
    boost::shared_ptr<Asset> loadTypeFromExt(std::string_view name);

    /**
     * Load an asset as the kind the caller holds it as, the type found from its extension.
     *
     * @return the asset, or null - for a file of a kind other than T as well, which is logged
     *         as that rather than looking like a missing file
     **/
    template <typename T>
    boost::shared_ptr<T> load(std::string_view name) {
        return as<T>(loadTypeFromExt(name), name);
    }

    /**
     * Load an asset of a named type as the kind the caller holds it as.
     **/
    template <typename T>
    boost::shared_ptr<T> load(std::string_view name, asset::Type t) {
        return as<T>(load(name, t), name);
    }

    /**
     * Load through a loader for the asset's type, and resolve a file to that type by any of
     * the extensions given. Registering a type a second time replaces the loader.
     *
     * @param extensions with the dot and in lower case, as ".png"; a file's own extension is
     *        lowered before it is looked up
     **/
    void registerLoader(const boost::shared_ptr<Loader>& loader, const std::vector<std::string>& extensions);

    /**
     * @return the loader registered for a type, or null
     **/
    boost::shared_ptr<Loader> resolveLoader(asset::Type t);

    /**
     * Where a name resolves to, for something that has to open the file itself rather than
     * load it as an asset.
     **/
    std::string path(std::string_view name) const;

 private:
    template <typename T>
    boost::shared_ptr<T> as(const boost::shared_ptr<Asset>& asset, std::string_view name) {
        if (!asset) {
            return boost::shared_ptr<T>();
        }
        boost::shared_ptr<T> held = boost::dynamic_pointer_cast<T>(asset);
        if (!held) {
            logger_->get()->error("{} loaded, and is not the {} it was asked for", name, typeid(T).name());
        }
        return held;
    }

    boost::filesystem::path path_;
    std::unordered_map<asset::Type, boost::shared_ptr<Loader>> loaders_;
    std::unordered_map<std::string, asset::Type> extensions_;
    boost::shared_ptr<v3d::log::Logger> logger_;
};

};  // namespace v3d::asset
