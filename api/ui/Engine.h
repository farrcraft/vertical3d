/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Engine.h>
#include <api/log/Logger.h>
#include <api/ui/Image.h>

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include <boost/json/object.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <entt/entt.hpp>


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
 * ui::Loader reads the document; this holds the result for as long as the app lives.
 **/
class Engine {
 public:
    /**
     * How a named image becomes something to draw.
     *
     * The library neither reads an image nor uploads one, and never interprets a source
     * name - an app resolves the source through its own asset manager and renderer. The
     * result can be part of a texture, so an app can serve many images out of one sprite
     * sheet; a bare handle converts to the whole of its texture.
     *
     * @return the image, or an unset one when the source could not be resolved
     **/
    typedef std::function<v3d::ui::Image(const std::string& source)> Resolve;

    Engine(const boost::shared_ptr<v3d::event::Engine>& eventEngine, const boost::shared_ptr<entt::dispatcher>& dispatcher,
        const boost::shared_ptr<v3d::log::Logger>& logger);

    bool load(const boost::json::object& config);

    /**
     * The dispatcher this ui sends its commands through, for something in the shell that
     * handles some of them itself.
     **/
    const boost::shared_ptr<entt::dispatcher>& dispatcher() const noexcept;

    /**
     * Hand every image the config named to a resolver and keep what comes back - the
     * image properties of every loaded theme, and every icon in every container.
     *
     * Safe to run again, and running it again is what resolves an icon whose source()
     * has changed since - each image is resolved from the name it holds now.
     *
     * A separate pass rather than part of load(), because an app has a renderer to
     * upload through only after the window is up, and because the same document is worth
     * loading whether or not anything will be drawn from it.
     *
     * @param resolve what turns a source into an image
     * @return how many sources were resolved to a set image
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
     * to know where a key goes.
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
     * Whether a component could be focused now: it is drawn, so neither it, its container nor
     * anything it is inside is hidden, and it is on a tab page that is up. A component hidden
     * while it holds the focus is no longer reachable, and keys pass it by.
     **/
    bool reachable(const boost::shared_ptr<Component>& component) const;

    /**
     * What the focus having moved is announced to.
     *
     * @param focused what the keyboard is now on, or null for nothing
     **/
    typedef std::function<void(const boost::shared_ptr<Component>& focused)> Focused;

    /**
     * Be told when the focus moves, which is how anything outside this library follows it.
     *
     * Three things move it and an app sees none of them directly - a press through
     * ui::Cursor, tab through ui::Keys, and focusFirst(). Each announces the move as it
     * happens, in the same frame, so text input can be started before the next character.
     *
     * Announced only when the focus actually changed, and after both components have been
     * told, so what is handed over is what focused() would answer - a component that did
     * not ask to be focusable is nothing, and nothing is what is announced.
     *
     * One listener, and the last caller wins. ui::shell::Keyboard sets it and clears it as
     * it is destroyed, so an app wanting one of its own sets it after the Keyboard is built
     * and clears it before the Keyboard is destroyed.
     *
     * @param moved what to call, or an empty function to stop being told
     **/
    void onFocus(const Focused& moved);

    /**
     * Move the focus to the next focusable component, or to the one before it.
     *
     * The order is the order the tree holds them in, which is the order they are drawn in:
     * containers as the config listed them, components by depth with add order between
     * equal depths, and a flow box's children in the order it was given them. A ui author
     * wanting a different tab order reorders the document. A hidden or disabled component
     * is skipped, and so is everything it holds.
     *
     * **A ui with nothing focused is left alone**, so a game's movement keys keep working:
     * tab must not take the focus onto the first widget of a hud nobody is looking at.
     *
     * When the component that held the focus is no longer reachable - hidden, disabled or
     * taken out of the tree since - there is no place in the order to move on from, so tab
     * starts again at the first component.
     *
     * @param forward whether to move to the next one rather than the previous one
     * @return whether the focus moved, which a ui holding one focusable component and a ui
     *         holding none both answer false
     **/
    bool focusNext(bool forward);

    /**
     * Put the focus on the first focusable component, which starts a screen being driven
     * from the keyboard.
     *
     * focusNext() leaves a ui with nothing focused alone, so without this a screen nobody
     * clicks on cannot be tabbed through. This is how a screen says it is keyboard driven:
     * the app calls it as the screen goes up, and a hud that keeps the movement keys
     * working does not.
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
     * resolveComponentImages() is also how an app resolves the one component it has just
     * pointed at a different source, without resolving the whole ui again.
     * @return how many images were set
     **/
    std::size_t resolveThemeImages(const Resolve& resolve);
    std::size_t resolveContainerImages(const Resolve& resolve);
    std::size_t resolveComponentImages(const Resolve& resolve, const boost::shared_ptr<Component>& component);

    /**
     * Resolve one component's image and write the whole answer onto it, so that resolving
     * again puts back the part of the texture as well as the texture.
     *
     * @param target anything with an image(Image) setter - an icon or a button
     * @return whether an image was set, which naming no image is not
     **/
    template <typename T>
    bool resolveIcon(const Resolve& resolve, const std::string& source, const boost::shared_ptr<T>& target);

    /**
     * What can be focused, in draw order, which is the order both focusFirst() and
     * focusNext() move through.
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
    Focused moved_;
};

};  // namespace v3d::ui
