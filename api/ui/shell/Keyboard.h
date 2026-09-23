/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/input/Keys.h>

#include <boost/shared_ptr.hpp>

#include <entt/entt.hpp>

union SDL_Event;

namespace v3d::render::realtime {
class Window;
};  // namespace v3d::render::realtime

namespace v3d::ui {
class Engine;
};  // namespace v3d::ui

namespace v3d::ui::shell {

/**
 * The platform half of ui::Keys: what turns an SDL event into the two calls that router
 * takes, and what an app's Engine::onEvent() hands every event to.
 *
 * It decodes the event, reads the modifiers off it, supplies the clipboard and starts the
 * platform composing, none of which ui::Keys does itself - ADR-0040, ADR-0028.
 *
 * A key the ui takes returns true, which keeps it from the command bound to it - ADR-0043.
 * Only a key going down is ever taken. A release always goes through, so a key held when a
 * box took the focus is still seen to come up and nothing is left stuck down.
 *
 * **Text input follows the focus.** The platform composes nothing until it is asked to, so
 * this turns it on while a text box holds the keyboard and off again after. It follows
 * ui::Engine::onFocus() rather than checking per event, because the focus also moves under
 * a mouse press this class never sees.
 *
 * The cursor stays the app's: ui::Cursor takes points rather than events, and an app routes
 * a press to it itself.
 **/
class Keyboard final {
 public:
    /**
     * @param ui where the focus lives, and what announces it moving
     * @param dispatcher where a focused component's event is sent
     * @param window what composes text, turned on and off as the focus reaches a text box
     *        and leaves it. A seam given none still edits: everything but the characters
     *        arrives as a key
     **/
    Keyboard(const boost::shared_ptr<Engine>& ui, const boost::shared_ptr<entt::dispatcher>& dispatcher,
        const boost::shared_ptr<v3d::render::realtime::Window>& window);

    ~Keyboard();

    Keyboard(const Keyboard&) = delete;
    Keyboard& operator=(const Keyboard&) = delete;

    /**
     * Offer one SDL event to the ui.
     *
     * @return whether the ui took it, which an app returns from onEvent() to keep it away
     *         from the bindings
     **/
    bool event(const SDL_Event& event);

    /**
     * The router underneath, for an app that has a key of its own to route - one that
     * arrived from somewhere other than the event loop, or a test driving the ui.
     **/
    input::Keys& keys() noexcept;

    /**
     * The platform's clipboard, which is what the four chords a text box answers read and
     * write. Public because it is the pair to hand any other ui::Keys an app builds.
     **/
    static input::Keys::Clipboard clipboard();

 private:
    /**
     * Compose text for as long as something is focused that is typed into, and not
     * otherwise. Called with wherever the focus landed.
     **/
    void follow(const boost::shared_ptr<Component>& focused);

    boost::shared_ptr<Engine> ui_;
    boost::shared_ptr<v3d::render::realtime::Window> window_;
    input::Keys keys_;
};

};  // namespace v3d::ui::shell
