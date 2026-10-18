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
     * name. An app resolves the source through its own asset manager and renderer. The
     * result can be part of a texture, so an app can serve many images out of one sprite
     * sheet. A bare handle converts to the whole of its texture.
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
     * Pass every image the config named to a resolver and keep what it returns. That covers
     * the image properties of every loaded theme, and every icon and button image in every
     * container.
     *
     * Safe to run again. Each image is resolved from the name it holds now, so running it
     * again picks up an icon whose source() has changed.
     *
     * A separate pass from load(), because an app has a renderer to upload through only
     * after the window is up, and a document can be loaded without anything being drawn.
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
     * One component has the focus at a time. The engine holds it because input::Cursor and
     * input::Keys both reach the engine. Cursor sets the focus as a press lands, and Keys
     * reads it to find where a key goes.
     *
     * @param component what to focus, or null for nothing. A component that is not
     *      focusable, or not usable, is treated as null. A press on a panel therefore takes
     *      the focus off, and does not move it onto the panel
     **/
    void focus(const boost::shared_ptr<Component>& component);

    /**
     * @return the component the keyboard is on, or null
     **/
    boost::shared_ptr<Component> focused() const;

    /**
     * Whether a component could be focused now. It is focusable and enabled, it is drawn
     * (neither it, its container nor anything it is inside is hidden), and it is on the chosen
     * tab page. input::Keys takes the focus off a focused component that is usable but not
     * reachable, which is one that was hidden or removed while it held the focus.
     **/
    bool reachable(const boost::shared_ptr<Component>& component) const;

    /**
     * Called when the focus moves.
     *
     * @param focused what the keyboard is now on, or null for nothing
     **/
    typedef std::function<void(const boost::shared_ptr<Component>& focused)> Focused;

    /**
     * Set the function called when the focus moves. Code outside this library follows the
     * focus through it.
     *
     * The focus moves on a press through input::Cursor, on tab or escape through input::Keys,
     * and on focusFirst(). An app sees none of these directly. The call happens as the focus moves,
     * in the same frame, so text input can be started before the next character.
     *
     * The call happens only when the focus changed, and after both components' focused()
     * flags have been set. It passes what focused() would return, so a request to focus a
     * component that is not focusable passes null.
     *
     * There is one listener, and each call replaces it. ui::shell::Keyboard sets it on
     * construction and clears it on destruction. An app that wants its own listener sets it
     * after the Keyboard is built, and clears it before the Keyboard is destroyed.
     *
     * @param moved what to call, or an empty function to stop the calls
     **/
    void onFocus(const Focused& moved);

    /**
     * Move the focus to the next focusable component, or to the one before it.
     *
     * The order is the draw order: containers as the config listed them, components by depth
     * with add order between equal depths, and a flow box's children in the order it holds
     * them. To change the tab order, reorder the document. A hidden or disabled component is
     * skipped, and so is everything it holds.
     *
     * **A ui with nothing focused is left alone**, so a game's movement keys keep working.
     * Tab does not move the focus onto the first widget of a hud.
     *
     * The component that held the focus may no longer be reachable, because it was hidden,
     * disabled or taken out of the tree. Then there is no place in the order to move on
     * from, so the focus goes to the first component, or to the last when moving backwards.
     *
     * @param forward whether to move to the next one rather than the previous one
     * @return whether the focus moved. It is false for a ui holding one focusable component
     *         and for a ui holding none
     **/
    bool focusNext(bool forward);

    /**
     * Put the focus on the first focusable component, so the screen can be driven from the
     * keyboard.
     *
     * focusNext() leaves a ui with nothing focused alone, so without this call a screen
     * cannot be tabbed through until something in it is clicked. An app calls this as a
     * keyboard-driven screen opens. A hud that leaves the movement keys to the game does not
     * call it.
     *
     * The order is focusNext()'s order, the draw order.
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
     * pointed at a different source, without resolving the whole ui again. A null component,
     * or a null entry in a container, resolves nothing.
     * @return how many images were set
     **/
    std::size_t resolveThemeImages(const Resolve& resolve);
    std::size_t resolveContainerImages(const Resolve& resolve);
    std::size_t resolveComponentImages(const Resolve& resolve, const boost::shared_ptr<Component>& component);

    /**
     * Resolve one component's image and write the whole result onto it, so that resolving
     * again restores the texture region as well as the texture.
     *
     * @param target anything with an image(Image) setter, such as an icon or a button
     * @return whether an image was set. A component that names no image returns false
     **/
    template <typename T>
    bool resolveIcon(const Resolve& resolve, const std::string& source, const boost::shared_ptr<T>& target);

    /**
     * What can be focused, in draw order. focusFirst() and focusNext() both move through
     * this order.
     **/
    std::vector<boost::shared_ptr<Component>> tabOrder() const;

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<v3d::event::Engine> eventEngine_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    std::vector<boost::shared_ptr<Container>> containers_;
    std::vector<boost::shared_ptr<style::Theme>> themes_;
    boost::shared_ptr<style::Theme> activeTheme_;
    // weak, because the component belongs to its container. The focus must not keep alive a
    // component whose container was unloaded
    boost::weak_ptr<Component> focused_;
    Focused moved_;
};

};  // namespace v3d::ui
