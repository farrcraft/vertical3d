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
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/system/error_code.hpp>

namespace v3d::asset {

/**
 **/
Manager::Manager(std::string_view path, const boost::shared_ptr<v3d::log::Logger>& logger) :
    Manager(std::vector<std::string>{ std::string(path) }, logger) {
}

/**
 **/
Manager::Manager(const std::vector<std::string>& roots, const boost::shared_ptr<v3d::log::Logger>& logger) :
    roots_(roots.begin(), roots.end()),
    logger_(logger) {
    if (roots_.empty()) {
        throw std::invalid_argument("an asset manager needs a root");
    }
    for (const boost::filesystem::path& root : roots_) {
        logger_->get()->info("Resolving assets under {}", root.string());
    }
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
    const std::string relative(name);
    for (const boost::filesystem::path& root : roots_) {
        const boost::filesystem::path candidate = root / relative;
        boost::system::error_code error;
        if (boost::filesystem::exists(candidate, error)) {
            return candidate.string();
        }
    }
    return (roots_.front() / relative).string();
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
