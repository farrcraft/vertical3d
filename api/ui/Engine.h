/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "Container.h"
#include "component/menu/Menu.h"
#include "style/Theme.h"

#include "../asset/Json.h"
#include "../event/Engine.h"
#include "../log/Logger.h"

#include <boost/json/object.hpp>
#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

namespace v3d::ui {

    class Engine {
     public:
        Engine(const boost::shared_ptr<v3d::event::Engine>& eventEngine, const boost::shared_ptr<entt::dispatcher>& dispatcher,
            const boost::shared_ptr<v3d::log::Logger>& logger);

        bool load(const boost::shared_ptr<v3d::asset::Json>& config);

        boost::shared_ptr<Container> container(const std::string_view& name);

        /**
         * Get a loaded theme by name.
         * @param name the theme name
         * @return the named theme, or null when no theme of that name was loaded
         **/
        boost::shared_ptr<style::Theme> theme(const std::string_view& name) const;
        /**
         * Get the active theme - the one components are drawn with.
         * @return the active theme, or null when no themes were loaded
         **/
        boost::shared_ptr<style::Theme> activeTheme() const;
        /**
         * Set the active theme.
         * @param name the name of the theme to make active
         * @return false when no theme of that name was loaded, leaving the active theme unchanged
         **/
        bool activeTheme(const std::string_view& name);

     protected:
         boost::shared_ptr<component::Menu> loadMenu(const boost::json::object& component);

     private:
        boost::shared_ptr<v3d::log::Logger> logger_;
        boost::shared_ptr<v3d::event::Engine> eventEngine_;
        boost::shared_ptr<entt::dispatcher> dispatcher_;
        std::vector<boost::shared_ptr<Container>> containers_;
        std::vector<boost::shared_ptr<style::Theme>> themes_;
        boost::shared_ptr<style::Theme> activeTheme_;
    };

};  // namespace v3d::ui
