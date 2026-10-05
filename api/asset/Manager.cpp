/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Manager.h"

#include <api/asset/loader/Json.h>
#include <api/asset/loader/Text.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::asset {

/**
 **/
Manager::Manager(std::string_view path, const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
    path_ = static_cast<std::string>(path);
    logger_->get()->info("Setting asset manager path to: {}", path);
    registerLoader(boost::make_shared<loader::Json>(logger_), {".json"});
    registerLoader(boost::make_shared<loader::Text>(logger_), {".txt"});
}

/**
 **/
void Manager::registerLoader(const boost::shared_ptr<Loader>& loader, const std::vector<std::string>& extensions) {
    loaders_[loader->type()] = loader;
    for (const std::string& extension : extensions) {
        extensions_[extension] = loader->type();
    }
}

/**
 **/
boost::shared_ptr<Loader> Manager::resolveLoader(asset::Type t) {
    auto search = loaders_.find(t);
    if (search == loaders_.end()) {
        throw std::invalid_argument("no loader for type");
    }
    return search->second;
}

/**
 **/
boost::shared_ptr<Asset> Manager::load(std::string_view name, asset::Type t) {
    boost::filesystem::path assetPath = path_;
    assetPath /= static_cast<std::string>(name);

    boost::shared_ptr<Loader> loader = resolveLoader(t);
    boost::shared_ptr<Asset> asset = loader->load(assetPath.string());
    return asset;
}

/**
 **/
boost::shared_ptr<Asset> Manager::loadTypeFromExt(std::string_view name) {
    std::string ext = boost::filesystem::path(static_cast<std::string>(name)).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    auto found = extensions_.find(ext);
    if (found == extensions_.end()) {
        throw std::invalid_argument("unrecognized extension");
    }
    return load(name, found->second);
}

};  // namespace v3d::asset
