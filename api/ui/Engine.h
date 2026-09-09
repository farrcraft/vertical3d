/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "../event/Engine.h"
#include "../log/Logger.h"
#include "../render/realtime/Handle.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <entt/entt.hpp>

namespace v3d::asset {
class Json;
};  // namespace v3d::asset

namespace v3d::ui {

class Component;
class Container;

namespace style {
class Theme;
};  // namespace style

/**
 * A loaded ui: the containers a config named, the themes it carried, and which of them is
 * active.
 *
 * Reading the document is ui::Loader's - a loader runs once and this is asked questions for
 * as long as the app lives, and keeping the two together put every component header and
 * boost::json in front of every app that draws a ui.
 **/
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
     * Put the keyboard on one component, taking it off whatever had it.
     *
     * One component at a time, and the engine is where that is decided because both
     * routers reach it: ui::Cursor gives the focus as a press lands and ui::Keys reads it
     * to know where a key goes. ADR-0040.
     *
     * @param component what to focus, or null for nothing. A component that did not ask
     *      to be focusable is nothing, so a press on a panel takes the focus off rather
     *      than moving it onto the panel
     **/
    void focus(const boost::shared_ptr<Component>& component);

    /**
     * @return the component the keyboard is on, or null
     **/
    boost::shared_ptr<Component> focused() const;

    /**
     * Move the focus to the next focusable component, or to the one before it, extending
     * ADR-0040 with a second way for the focus to move.
     *
     * The order is the order the tree holds them in, which is the order they are drawn in:
     * containers as the config listed them, components by depth with add order between
     * equal depths, and a flow box's children in the order it was given them. A ui author
     * wanting a different tab order reorders the document. A hidden component is skipped,
     * and so is everything it holds.
     *
     * **A ui with nothing focused is left alone**, which is what keeps a game's movement
     * keys working: tab must not take the focus onto the first widget of a hud nobody is
     * looking at.
     *
     * @param forward whether to move to the next one rather than the previous one
     * @return whether the focus moved, which a ui holding one focusable component and a ui
     *         holding none both answer false
     **/
    bool focusNext(bool forward);

    /**
     * Put the focus on the first focusable component, which is what starts a screen being
     * driven from the keyboard.
     *
     * focusNext() deliberately leaves a ui with nothing focused alone, so a press was the
     * only thing that ever gave out a first focus and a screen nobody clicks on could not
     * be tabbed through at all. This is how a screen says it is keyboard driven: the app
     * calls it as the screen goes up, and a hud that would rather keep the movement keys
     * working simply does not.
     *
     * The order is focusNext()'s order - the order things are drawn in.
     *
     * @return whether anything was focused, false when the ui holds nothing focusable
     **/
    bool focusFirst();

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

    /**
     * Resolve the images the loaded themes name, and the ones the loaded components do.
     * @return how many handles were set
     **/
    std::size_t resolveThemeImages(const Resolve& resolve);
    std::size_t resolveContainerImages(const Resolve& resolve);
    std::size_t resolveComponentImages(const Resolve& resolve, const boost::shared_ptr<Component>& component);

    /**
     * Resolve one component's image and write the handle onto it.
     *
     * @param target anything with a texture(handle) setter - an icon or a button
     * @return whether a handle was set, which naming no image is not
     **/
    template <typename T>
    bool resolveIcon(const Resolve& resolve, const std::string& source, const boost::shared_ptr<T>& target);

    /**
     * What can be focused, in the order the draw walk reaches it - the one order both
     * focusFirst() and focusNext() move through.
     **/
    std::vector<boost::shared_ptr<Component>> tabOrder() const;

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<v3d::event::Engine> eventEngine_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    std::vector<boost::shared_ptr<Container>> containers_;
    std::vector<boost::shared_ptr<style::Theme>> themes_;
    boost::shared_ptr<style::Theme> activeTheme_;
    // held rather than owned, for the reason ui::Cursor holds a press that way: the
    // component belongs to its container, and a focus outliving one that was unloaded
    // should not keep it alive
    boost::weak_ptr<Component> focused_;
};

};  // namespace v3d::ui
