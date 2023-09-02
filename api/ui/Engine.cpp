/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

#include <string>

#include <boost/make_shared.hpp>

namespace v3d::ui {
    bool Engine::load(const boost::shared_ptr<v3d::log::Logger>& logger, boost::shared_ptr<v3d::asset::Json>& config) {
        auto const doc = config->document();

        // read themes
        auto const themesSection = doc.at("themes");
        if (!themesSection.is_array()) {
            LOG_ERROR(logger) << "Missing themes in config";
            return false;
        }
        auto const themes = themesSection.as_array();
        auto it = themes.begin();
        for (; it != themes.end(); ++it) {
            if (!it->is_object()) {
                LOG_ERROR(logger) << "Unrecognized theme config";
                return false;
            }
            auto const themeEntry = it->as_object();
            std::string themeName = boost::json::value_to<std::string>(themeEntry.at("name"));
            boost::shared_ptr<style::Theme> theme = boost::make_shared<style::Theme>(themeName);
            themes_.push_back(theme);
        }

        // read containers
        auto const containersSection = doc.at("containers");
        if (!containersSection.is_array()) {
            LOG_ERROR(logger) << "Missing containers in config";
            return false;
        }
        auto const containers = containersSection.as_array();
        auto containerIterator = containers.begin();
        for (; containerIterator != containers.end(); ++containerIterator) {
            if (!containerIterator->is_object()) {
                LOG_ERROR(logger) << "Unrecognized containers config";
                return false;
            }
            auto const containerEntry = containerIterator->as_object();
            std::string containerName = boost::json::value_to<std::string>(containerEntry.at("name"));
            boost::shared_ptr<Container> container = boost::make_shared<Container>(containerName);
            containers_.push_back(container);
        }
    }
};  // namespace v3d::ui
