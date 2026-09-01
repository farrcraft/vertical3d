/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

#include <string>
#include <vector>

#include "component/menu/MenuItem.h"

#include <boost/make_shared.hpp>

namespace v3d::ui {
    Engine::Engine(const boost::shared_ptr<v3d::event::Engine>& eventEngine, const boost::shared_ptr<entt::dispatcher>& dispatcher,
        const boost::shared_ptr<v3d::log::Logger>& logger) :
        eventEngine_(eventEngine), dispatcher_(dispatcher), logger_(logger) {
    }

    bool Engine::load(const boost::shared_ptr<v3d::asset::Json>& config) {
        auto const doc = config->document();

        // read themes
        auto const themesSection = doc.at("themes");
        if (!themesSection.is_array()) {
            logger_->get()->error("Missing themes in config");
            return false;
        }
        auto const themes = themesSection.as_array();
        auto it = themes.begin();
        for (; it != themes.end(); ++it) {
            if (!it->is_object()) {
                logger_->get()->error("Unrecognized theme config");
                return false;
            }
            auto const themeEntry = it->as_object();
            std::string themeName = boost::json::value_to<std::string>(themeEntry.at("name"));
            boost::shared_ptr<style::Theme> theme = boost::make_shared<style::Theme>(themeName);
            themes_.push_back(theme);
            // the config has no field naming the active theme yet - that arrives with the style
            // schema (docs/LuxaAudit.md), so until then the first theme loaded is the active one.
            if (!activeTheme_) {
                activeTheme_ = theme;
            }
        }

        // read containers
        auto const containersSection = doc.at("containers");
        if (!containersSection.is_array()) {
            logger_->get()->error("Missing containers in config");
            return false;
        }
        auto const containers = containersSection.as_array();
        auto containerIterator = containers.begin();
        for (; containerIterator != containers.end(); ++containerIterator) {
            if (!containerIterator->is_object()) {
                logger_->get()->error("Unrecognized containers config");
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
                logger_->get()->error("Missing components in config");
                return false;
            }
            auto const components = componentsSection.as_array();
            auto componentIterator = components.begin();
            for (; componentIterator != components.end(); ++componentIterator) {
                if (!componentIterator->is_object()) {
                    logger_->get()->error("Unrecognized component config");
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
                    // this is the menu the app navigates, so it starts as its own active level
                    menu->level(menu);
                    container->add(menu);
                }
            }
        }
        return true;
    }

    /**
     **/
    boost::shared_ptr<component::Menu> Engine::loadMenu(const boost::json::object& component) {
        boost::shared_ptr<component::Menu> menu = boost::make_shared<component::Menu>(dispatcher_);

        auto const itemsSection = component.at("items");
        if (!itemsSection.is_array()) {
            logger_->get()->error("Missing menu items in config");
            return nullptr;
        }
        auto const items = itemsSection.as_array();
        auto itemsIterator = items.begin();
        for (; itemsIterator != items.end(); ++itemsIterator) {
            if (!itemsIterator->is_object()) {
                logger_->get()->error("Unrecognized menu item config");
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
            // the owning menu has to be set before the submenu below, which reads it to find its parent
            menuItem->menu(menu);
            if (context.length() > 0 && command.length() > 0) {
                boost::shared_ptr<v3d::event::Context> eventContext = eventEngine_->resolveContext(context);
                v3d::event::Event event(command, eventContext);
                menuItem->event(event);
            }

            menu->addItem(menuItem);

            if (menuItem->type() == menu::ItemType::Submenu) {
                boost::shared_ptr<component::Menu> submenu = loadMenu(menuItemConfig);
                if (!submenu) {  // submenu(null) would fault setting the parent
                    return nullptr;
                }
                menuItem->submenu(submenu);
            }
        }
        menu->active(0);
        return menu;
    }

    /**
     **/
    boost::shared_ptr<style::Theme> Engine::theme(const std::string_view& name) const {
        auto it = themes_.begin();
        for (; it != themes_.end(); ++it) {
            if ((*it)->name() == name) {
                return *it;
            }
        }
        return nullptr;
    }

    /**
     **/
    boost::shared_ptr<style::Theme> Engine::activeTheme() const {
        return activeTheme_;
    }

    /**
     **/
    bool Engine::activeTheme(const std::string_view& name) {
        boost::shared_ptr<style::Theme> found = theme(name);
        if (!found) {
            logger_->get()->error("No such ui theme [{}]", name);
            return false;
        }
        activeTheme_ = found;
        return true;
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
    /**
     **/
    const std::vector<boost::shared_ptr<Container>>& Engine::containers() const noexcept {
        return containers_;
    }

};  // namespace v3d::ui
