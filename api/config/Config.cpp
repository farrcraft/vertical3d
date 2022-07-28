/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Config.h"

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
        auto const configs = doc.at("configs");
        if (!configs.is_array()) {
            LOG_ERROR(logger_) << "Missing configs in config";
            return false;
        }
        // for each context
        auto const items = configs.as_array();
        auto it = items.begin();
        for (; it != items.end(); ++it) {
            if (!it->is_object()) {
                LOG_ERROR(logger_) << "Unrecognized config";
                return false;
            }
            auto const entry = it->as_object();
            std::string typeName = boost::json::value_to<std::string>(entry.at("type"));
            std::string fileName = boost::json::value_to<std::string>(entry.at("file"));
            Type type = stringToType(typeName);
            if (type == Type::Unknown) {
                LOG_ERROR(logger_) << "Unknown config type: " << typeName;
                return false;
            }
            boost::shared_ptr<v3d::asset::Json> asset = boost::dynamic_pointer_cast<v3d::asset::Json>(assetManager->loadTypeFromExt(fileName));
            if (!asset) {
                LOG_ERROR(logger_) << "Config file not found: " << fileName;
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

    /**

    bool Config::loadBindings(const boost::shared_ptr<v3d::asset::Json>& bindings) {
        auto const doc = bindings->document();
        auto const contexts = doc.at("contexts");
        if (!contexts.is_array()) {
            LOG_ERROR(logger_) << "Missing contexts in bindings config";
            return false;
        }
        // for each context
        auto const items = contexts.as_array();
        auto it = items.begin();
        for (; it != items.end(); ++it) {
            if (!it->is_object()) {
                LOG_ERROR(logger_) << "Unrecognized context config";
                return false;
            }
            auto const context = it->as_object();
            auto const name = boost::json::value_to<std::string>(context.at("context"));
            auto binding = boost::make_shared<BindingContext>(name);

            auto const bindingsJson = context.at("bindings");
            if (!bindingsJson.is_object()) {
                LOG_ERROR(logger_) << "Missing bindings config";
                return false;
            }
            // for each binding
            auto const bindingsObj = bindingsJson.as_object();
            auto bindingsIter = bindingsObj.begin();
            for (; bindingsIter != bindingsObj.end(); ++bindingsIter) {
                std::string input = bindingsIter->key();
                // we're expecting string values only
                auto const actionValue = bindingsIter->value();
                if (!actionValue.is_string()) {
                    LOG_ERROR(logger_) << "Unrecognized binding value type";
                    return false;
                }
                binding->setBinding(input, actionValue.as_string());
            }
            LOG_INFO(logger_) << bindingsObj.size() << " bindings loaded";

            contexts_[name] = binding;
        }
        LOG_INFO(logger_) << contexts_.size() << " contexts loaded";

        return true;
    }
    */
};  // namespace v3d::config
