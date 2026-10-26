/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Manager.h"

#include <api/asset/loader/Json.h>
#include <api/asset/loader/Text.h>
#include <api/log/Logger.h>

#include "Asset.h"
#include "Loader.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <typeindex>
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
    loaders_[loader->kind()] = loader;
    for (const std::string& extension : extensions) {
        extensions_[extension] = loader;
    }
}

/**
 **/
boost::shared_ptr<Loader> Manager::resolveLoader(std::type_index kind) const {
    auto search = loaders_.find(kind);
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
boost::shared_ptr<Loader> Manager::forExtension(std::string_view name) const {
    std::string ext = boost::filesystem::path(static_cast<std::string>(name)).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    auto found = extensions_.find(ext);
    if (found == extensions_.end()) {
        return boost::shared_ptr<Loader>();
    }
    return found->second;
}

/**
 **/
boost::shared_ptr<Asset> Manager::load(std::string_view name) {
    const boost::shared_ptr<Loader> loader = forExtension(name);
    if (!loader) {
        logger_->get()->error("Nothing is registered to load the extension of {}, so it is not loaded", name);
        return boost::shared_ptr<Asset>();
    }
    return loader->load(path(name));
}

/**
 **/
boost::shared_ptr<Asset> Manager::load(std::string_view name, std::type_index kind) {
    boost::shared_ptr<Loader> loader = forExtension(name);
    if (!loader) {
        loader = resolveLoader(kind);
    }
    if (!loader) {
        logger_->get()->error("Nothing is registered to load {} as a {}", name, kind.name());
        return boost::shared_ptr<Asset>();
    }
    return loader->load(path(name));
}

};  // namespace v3d::asset
