/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <string>
#include <string_view>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "Asset.h"
#include "Loader.h"

#include <boost/filesystem.hpp>
#include <boost/pointer_cast.hpp>

namespace v3d::asset {
/**
 * The Asset Manager provides an access point for mapping and loading assets
 * within a single path.
 *
 * It loads the kinds a loader has been registered for. A manager starts with the document
 * loaders, Json and Text. The loaders for pictures and models are in api/asset/media, and the
 * loader for sound is in api/audio. Each library registers its own.
 *
 * A file's extension picks its loader. A caller that names a kind gets null when that loader
 * produces another kind, and a file whose extension no loader reads is loaded by the loader
 * registered for the kind named.
 *
 * **Every way of loading reports a failure the same way: no asset, and a line in the log.**
 * That covers a missing file, a file that does not decode, a kind with no registered loader
 * and an unregistered extension. A caller checks one pointer whatever went wrong.
 **/
class Manager final {
 public:
    /**
     **/
    Manager(std::string_view path, const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Load an asset through the loader registered for its extension.
     *
     * @return the asset, or null
     **/
    boost::shared_ptr<Asset> load(std::string_view name);

    /**
     * Load an asset as the kind the caller holds it as.
     *
     * @return the asset, or null. A file that loads as a kind other than T also gives null,
     *         and the log says so, so it does not look like a missing file
     **/
    template <typename T>
    boost::shared_ptr<T> load(std::string_view name) {
        return as<T>(load(name, typeid(T)), name);
    }

    /**
     * Load through a loader for the kind it produces, and resolve a file to it by any of the
     * extensions given. A second loader of the same kind replaces the first for a file whose
     * extension no loader reads, and an extension registered again moves to the new loader.
     *
     * @param extensions with the dot and in lower case, as ".png"; a file's own extension is
     *        lowered before it is looked up
     **/
    void registerLoader(const boost::shared_ptr<Loader>& loader, const std::vector<std::string>& extensions);

    /**
     * @return the loader registered last for a kind, or null
     **/
    boost::shared_ptr<Loader> resolveLoader(std::type_index kind) const;

    /**
     * @return the loader registered last for kind T, or null
     **/
    template <typename T>
    boost::shared_ptr<Loader> resolveLoader() const {
        return resolveLoader(typeid(T));
    }

    /**
     * The path a name resolves to, for a caller that opens the file itself instead of loading
     * it as an asset.
     **/
    std::string path(std::string_view name) const;

 private:
    /**
     * Load through the loader for the extension, or for the kind when no loader reads the
     * extension.
     **/
    boost::shared_ptr<Asset> load(std::string_view name, std::type_index kind);

    /**
     * @return the loader registered for the extension of a name, or null
     **/
    boost::shared_ptr<Loader> forExtension(std::string_view name) const;

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
    std::unordered_map<std::type_index, boost::shared_ptr<Loader>> loaders_;
    std::unordered_map<std::string, boost::shared_ptr<Loader>> extensions_;
    boost::shared_ptr<v3d::log::Logger> logger_;
};

};  // namespace v3d::asset
