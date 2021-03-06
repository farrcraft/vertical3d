/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Config.h"

#include <string>

#include <boost/make_shared.hpp>

namespace odyssey::config {
    /**
     **/
    Config::Config(const boost::shared_ptr<v3d::log::Logger>& logger) :
        logger_(logger) {
    }

    /**
     **/
    bool Config::load(const boost::shared_ptr<v3d::asset::Manager>& assetManager) {
        boost::shared_ptr<v3d::asset::Json> config = boost::dynamic_pointer_cast<v3d::asset::Json>(assetManager->loadTypeFromExt("config.json"));
        if (!config || !loadConfig(config)) {
            return false;
        }

        boost::shared_ptr<v3d::asset::Json> bindings = boost::dynamic_pointer_cast<v3d::asset::Json>(assetManager->loadTypeFromExt("bindings.json"));
        if (!bindings || !loadBindings(bindings)) {
            return false;
        }
        return true;
    }

    /**
     **/
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

    /**
     **/
    bool Config::loadConfig(const boost::shared_ptr<v3d::asset::Json>& config) {
        auto const doc = config->document();
        // default window options
        auto const window = doc.at("window");
        windowWidth_ = boost::json::value_to<int>(window.at("width"));
        windowHeight_ = boost::json::value_to<int>(window.at("height"));
        LOG_INFO(logger_) << "Setting default window size to " << windowWidth_ << "x" << windowHeight_;

        return true;
    }

    /**
     **/
    int Config::windowWidth() const {
        return windowWidth_;
    }

    /**
     **/
    int Config::windowHeight() const {
        return windowHeight_;
    }
};  // namespace odyssey::config
