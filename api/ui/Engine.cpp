/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

#include <string>

#include "component/menu/MenuItem.h"

#include <boost/make_shared.hpp>

namespace v3d::ui {
    Engine::Engine(const boost::shared_ptr<v3d::event::Engine>& eventEngine, const boost::shared_ptr<v3d::log::Logger>& logger) :
        eventEngine_(eventEngine), logger_(logger) {
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
                std::string componentName = boost::json::value_to<std::string>(componentEntry.at("name"));

                // read individual component types
                if (componentType == "menu") {
                    boost::shared_ptr<component::Menu> menu = loadMenu(componentEntry);
                    if (!menu) {
                        return false;
                    }
                    menu->name(componentName);
                    container->add(menu);
                }
            }
        }
        return true;
    }

    /**
     **/
    boost::shared_ptr<component::Menu> Engine::loadMenu(const boost::json::object& component) {
        boost::shared_ptr<component::Menu> menu = boost::make_shared<component::Menu>();

        auto const itemsSection = component.at("items");
        if (!itemsSection.is_array()) {
            LOG_ERROR(logger_) << "Missing menu items in config";
            return nullptr;
        }
        auto const items = itemsSection.as_array();
        auto itemsIterator = items.begin();
        for (; itemsIterator != items.end(); ++itemsIterator) {
            if (!itemsIterator->is_object()) {
                LOG_ERROR(logger_) << "Unrecognized menu item config";
                return nullptr;
            }
            auto const menuItemConfig = itemsIterator->as_object();
            std::string label = boost::json::value_to<std::string>(menuItemConfig.at("label"));
            std::string itemType = boost::json::value_to<std::string>(menuItemConfig.at("type"));

            std::string command;
            std::string context;
            if (menuItemConfig.contains("command")) {
                command = boost::json::value_to<std::string>(menuItemConfig.at("command"));
            }
            if (menuItemConfig.contains("context")) {
                context = boost::json::value_to<std::string>(menuItemConfig.at("context"));
            }

            boost::shared_ptr<component::MenuItem> menuItem = boost::make_shared<component::MenuItem>(menu::stringToType(itemType), label);
            if (context.length() > 0 && command.length() > 0) {
                boost::shared_ptr<v3d::event::Context> eventContext = eventEngine_->resolveContext(context);
                v3d::event::Event event(command, eventContext);
                menuItem->event(event);
            }

            menu->addItem(menuItem);

            if (menuItem->type() == menu::ItemType::Submenu) {
                boost::shared_ptr<component::Menu> submenu = loadMenu(menuItemConfig);
                menuItem->submenu(submenu);
            }
        }
        return menu;
    }

    boost::shared_ptr<Container> Engine::container(const std::string_view& name) {
        auto containers = containers_.begin();
        for (; containers != containers_.end(); ++containers) {
            if ((*containers)->name() == name) {
                return *containers;
            }
        }
        return nullptr;
    }
};  // namespace v3d::ui
