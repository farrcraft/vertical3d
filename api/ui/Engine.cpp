/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

#include <string>

#include "component/menu/Menu.h"
#include "component/menu/MenuItem.h"

#include <boost/make_shared.hpp>

namespace v3d::ui {
    Engine::Engine(const boost::shared_ptr<v3d::log::Logger>& logger) : logger_(logger) {
    }

    bool Engine::load(const boost::shared_ptr<v3d::asset::Json>& config) {
        auto const doc = config->document();

        // read themes
        auto const themesSection = doc.at("themes");
        if (!themesSection.is_array()) {
            LOG_ERROR(logger_) << "Missing themes in config";
            return false;
        }
        auto const themes = themesSection.as_array();
        auto it = themes.begin();
        for (; it != themes.end(); ++it) {
            if (!it->is_object()) {
                LOG_ERROR(logger_) << "Unrecognized theme config";
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
            LOG_ERROR(logger_) << "Missing containers in config";
            return false;
        }
        auto const containers = containersSection.as_array();
        auto containerIterator = containers.begin();
        for (; containerIterator != containers.end(); ++containerIterator) {
            if (!containerIterator->is_object()) {
                LOG_ERROR(logger_) << "Unrecognized containers config";
                return false;
            }
            auto const containerEntry = containerIterator->as_object();
            std::string containerName = boost::json::value_to<std::string>(containerEntry.at("name"));
            bool visible = boost::json::value_to<bool>(containerEntry.at("visible"));
            boost::shared_ptr<Container> container = boost::make_shared<Container>(containerName, visible);
            containers_.push_back(container);

            // read components in this container
            auto const componentsSection = containerEntry.at("components");
            if (!componentsSection.is_array()) {
                LOG_ERROR(logger_) << "Missing components in config";
                return false;
            }
            auto const components = componentsSection.as_array();
            auto componentIterator = components.begin();
            for (; componentIterator != components.end(); ++componentIterator) {
                if (!componentIterator->is_object()) {
                    LOG_ERROR(logger_) << "Unrecognized component config";
                    return false;
                }
                auto const componentEntry = componentIterator->as_object();
                std::string componentType = boost::json::value_to<std::string>(componentEntry.at("type"));

                // read individual component types
                if (componentType == "menu") {
                    if (!loadMenu(componentEntry)) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    /**
     **/
    bool Engine::loadMenu(const boost::json::object& component) {
        boost::shared_ptr<Menu> menu = boost::make_shared<Menu>();

        auto const itemsSection = component.at("items");
        if (!itemsSection.is_array()) {
            LOG_ERROR(logger_) << "Missing menu items in config";
            return false;
        }
        auto const items = itemsSection.as_array();
        auto itemsIterator = items.begin();
        for (; itemsIterator != items.end(); ++itemsIterator) {
            if (!itemsIterator->is_object()) {
                LOG_ERROR(logger_) << "Unrecognized menu item config";
                return false;
            }
            auto const menuItemConfig = itemsIterator->as_object();
            std::string label = boost::json::value_to<std::string>(menuItemConfig.at("label"));
            std::string itemType = boost::json::value_to<std::string>(menuItemConfig.at("type"));
            std::string command = boost::json::value_to<std::string>(menuItemConfig.at("command"));
            std::string scope = boost::json::value_to<std::string>(menuItemConfig.at("scope"));

            boost::shared_ptr<MenuItem> menuItem = boost::make_shared<MenuItem>(menu::stringToType(itemType), label, command, scope);
            menu->addItem(menuItem);
        }
        return true;
    }
};  // namespace v3d::ui
