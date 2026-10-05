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
        return boost::shared_ptr<Loader>();
    }
    return search->second;
}

/**
 **/
std::string Manager::path(std::string_view name) const {
    return (path_ / static_cast<std::string>(name)).string();
}

/**
 **/
boost::shared_ptr<Asset> Manager::load(std::string_view name, asset::Type t) {
    boost::shared_ptr<Loader> loader = resolveLoader(t);
    if (!loader) {
        logger_->get()->error("Nothing is registered to load {} as type {}", name, static_cast<int>(t));
        return boost::shared_ptr<Asset>();
    }
    return loader->load(path(name));
}

/**
 **/
boost::shared_ptr<Asset> Manager::loadTypeFromExt(std::string_view name) {
    std::string ext = boost::filesystem::path(static_cast<std::string>(name)).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    auto found = extensions_.find(ext);
    if (found == extensions_.end()) {
        logger_->get()->error("Nothing is registered to load a {} file, so {} is not loaded", ext.empty() ? "nameless" : ext, name);
        return boost::shared_ptr<Asset>();
    }
    return load(name, found->second);
}

};  // namespace v3d::asset
