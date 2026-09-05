/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Config.h"

#include <exception>
#include <string>

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
    boost::shared_ptr<v3d::asset::Json> config = boost::dynamic_pointer_cast<v3d::asset::Json>(assetManager->loadTypeFromExt("config.json"));
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
    // for each context
    auto const items = configs.as_array();
    auto it = items.begin();
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
        std::string typeName = boost::json::value_to<std::string>(entry.at("type"));
        std::string fileName = boost::json::value_to<std::string>(entry.at("file"));
        Type type = stringToType(typeName);
        if (type == Type::Unknown) {
            logger_->get()->error("Unknown config type: {}", typeName);
            return false;
        }
        // loadTypeFromExt throws for an extension it has no loader for, which is the one
        // way a config file can reject this function rather than being rejected by it.
        boost::shared_ptr<v3d::asset::Json> asset;
        try {
            asset = boost::dynamic_pointer_cast<v3d::asset::Json>(assetManager->loadTypeFromExt(fileName));
        }
        catch (std::exception const& e) {
            logger_->get()->error("Config file could not be loaded: {} - {}", fileName, e.what());
            return false;
        }
        if (!asset) {
            logger_->get()->error("Config file not found: {}", fileName);
            return false;
        }
        configs_[type] = asset;
    }
    return true;
}

/**
 **/
boost::shared_ptr<v3d::asset::Json> Config::get(Type configType) {
    auto entry = configs_.find(configType);
    if (entry == configs_.end()) {
        return nullptr;
    }
    return entry->second;
}

};  // namespace v3d::config
