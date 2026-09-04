/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "Container.h"
#include "component/Button.h"
#include "component/Icon.h"
#include "component/Label.h"
#include "component/Toolbar.h"
#include "component/menu/Menu.h"
#include "component/menu/MenuBar.h"
#include "style/Theme.h"

#include "../asset/Json.h"
#include "../event/Engine.h"
#include "../log/Logger.h"
#include "../render/realtime/Handle.h"

#include <boost/json/object.hpp>
#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

namespace v3d::ui {

    class Engine {
     public:
        /**
         * How a named image becomes a texture.
         *
         * The library neither reads an image nor uploads one - an app resolves the source
         * through its own asset manager and renderer, per ADR-0020.
         *
         * @return the texture, or an unset handle when the source could not be resolved
         **/
        typedef std::function<v3d::render::realtime::TextureHandle(const std::string& source)> Resolve;

        Engine(const boost::shared_ptr<v3d::event::Engine>& eventEngine, const boost::shared_ptr<entt::dispatcher>& dispatcher,
            const boost::shared_ptr<v3d::log::Logger>& logger);

        bool load(const boost::shared_ptr<v3d::asset::Json>& config);

        /**
         * Hand every image the config named to a resolver and keep what comes back - the
         * image properties of every loaded theme, and every icon in every container.
         *
         * A separate pass rather than part of load(), because an app has a renderer to
         * upload through only after the window is up, and because the same document is worth
         * loading whether or not anything will be drawn from it.
         *
         * @param resolve what turns a source into a texture
         * @return how many sources were resolved to a set handle
         **/
        std::size_t resolveImages(const Resolve& resolve);

        boost::shared_ptr<Container> container(const std::string_view& name);

        /**
         * Get every container that was loaded, in the order the config listed them.
         * @return the containers, for a renderer that has to walk all of them
         **/
        const std::vector<boost::shared_ptr<Container>>& containers() const noexcept;

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
         boost::shared_ptr<component::MenuBar> loadMenuBar(const boost::json::object& component);
         boost::shared_ptr<component::Toolbar> loadToolbar(const boost::json::object& component);
         boost::shared_ptr<component::Button> loadButton(const boost::json::object& component);
         boost::shared_ptr<component::Label> loadLabel(const boost::json::object& component);
         boost::shared_ptr<component::Icon> loadIcon(const boost::json::object& component);

         /**
          * Read one style and everything in it into a theme.
          * @return false when the style names a class it cannot be built as
          **/
         bool loadStyle(const boost::json::object& entry, const boost::shared_ptr<style::Theme>& theme);

         /**
          * Read the four kinds of property a style may hold - colours, numbers, fonts and
          * images - each from its own array.
          **/
         bool loadProperties(const boost::json::object& entry, const boost::shared_ptr<Style>& target);

         /**
          * Read what every component may carry whatever its type: where it is, how big it is,
          * which style draws it, and whether it is drawn at all.
          **/
         void loadAttributes(const boost::json::object& entry, const boost::shared_ptr<Component>& component);

         /**
          * Resolve one component's image and write the handle onto it.
          *
          * @param target anything with a texture(handle) setter - an icon or a button
          * @return whether a handle was set, which naming no image is not
          **/
         template <typename T>
         bool resolveIcon(const Resolve& resolve, const std::string& source, const boost::shared_ptr<T>& target);

         /**
          * Read the "context" and "command" pair a menu item or a toolbar button names, and
          * resolve the context.
          * @return the event, or one with no context when the config gave neither
          **/
         v3d::event::Event loadCommand(const boost::json::object& entry);

     private:
        boost::shared_ptr<v3d::log::Logger> logger_;
        boost::shared_ptr<v3d::event::Engine> eventEngine_;
        boost::shared_ptr<entt::dispatcher> dispatcher_;
        std::vector<boost::shared_ptr<Container>> containers_;
        std::vector<boost::shared_ptr<style::Theme>> themes_;
        boost::shared_ptr<style::Theme> activeTheme_;
    };

};  // namespace v3d::ui
