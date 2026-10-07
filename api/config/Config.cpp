/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Config.h"

#include <api/asset/Json.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/make_shared.hpp>

namespace v3d::config {
/**
 **/
Config::Config(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
}

/**
 **/
bool Config::load(const boost::shared_ptr<v3d::asset::Manager>& assetManager) {
    boost::shared_ptr<v3d::asset::kind::Json> config = assetManager->load<v3d::asset::kind::Json>("config.json");
    if (!config) {
        return false;
    }
    auto const doc = config->document();
    // every lookup below is guarded by a contains() rather than reaching straight for
    // at(): boost::json::at throws, and a config this function does not understand has to
    // come back as a false return like every other rejection here, not as an exception
    // out of engine startup.
    if (!doc.contains("configs")) {
        logger_->get()->error("Missing configs in config");
        return false;
    }
    auto const configs = doc.at("configs");
    if (!configs.is_array()) {
        logger_->get()->error("Missing configs in config");
        return false;
    }
    // filed here and kept only once every entry has loaded, so a failed load files nothing
    std::unordered_map<std::string, boost::shared_ptr<v3d::asset::kind::Json> > read;
    auto const items = configs.as_array();
    const auto* it = items.begin();
    for (; it != items.end(); ++it) {
        if (!it->is_object()) {
            logger_->get()->error("Unrecognized config");
            return false;
        }
        auto const entry = it->as_object();
        if (!entry.contains("type") || !entry.contains("file")) {
            logger_->get()->error("Config entry needs both a type and a file");
            return false;
        }
        const std::optional<std::string> type = v3d::asset::readString(entry, "type");
        const std::optional<std::string> file = v3d::asset::readString(entry, "file");
        if (!type || !file) {
            logger_->get()->error("Config entry gives a type or a file that is not a string");
            return false;
        }
        const std::string& typeName = *type;
        const std::string& fileName = *file;
        // the empty name is what typeName gives Type::Unknown, so a document filed under it
        // would be found by get(Type::Unknown)
        if (typeName.empty()) {
            logger_->get()->error("Config entry for {} has an empty type", fileName);
            return false;
        }
        // a type the api does not read is the app's, and is filed for it to ask for by name
        if (stringToType(typeName) == Type::Unknown) {
            logger_->get()->debug("Config names a {} document, which the api does not read", typeName);
        }
        // a file that is missing, is not json, or names an extension nothing loads is no
        // asset, and the manager has logged which
        const boost::shared_ptr<v3d::asset::kind::Json> asset = assetManager->load<v3d::asset::kind::Json>(fileName);
        if (!asset) {
            logger_->get()->error("Config file could not be loaded: {}", fileName);
            return false;
        }
        read[typeName] = asset;
    }
    configs_ = std::move(read);
    return true;
}

/**
 **/
boost::shared_ptr<v3d::asset::kind::Json> Config::get(Type configType) {
    return get(typeName(configType));
}

/**
 **/
boost::shared_ptr<v3d::asset::kind::Json> Config::get(std::string_view type) {
    auto entry = configs_.find(std::string(type));
    if (entry == configs_.end()) {
        return boost::shared_ptr<v3d::asset::kind::Json>();
    }
    return entry->second;
}

};  // namespace v3d::config
