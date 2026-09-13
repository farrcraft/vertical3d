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
 * ui::Keys names no platform type on purpose - a key is a name and a character is utf-8,
 * per ADR-0040 - which leaves an app to decode the event, read the modifiers off it, find a
 * clipboard and start the platform composing before any of it works. That is the same
 * decoding in every app, so it is the api's rather than each app's, per ADR-0028.
 *
 * It goes in an app's onEvent() because ADR-0043 put the app ahead of the bindings: a key
 * the ui took must not also fire the command bound to it, and returning true is what stops
 * it. Only a key going down is ever taken. A release always goes through, so a key held
 * when a box took the focus is still seen to come up and nothing is left stuck down.
 *
 * **Text input follows the focus.** The platform composes nothing until it is asked to, so
 * this turns it on while a text box holds the keyboard and off again after - which is what
 * ADR-0040 recorded as a consequence it had not paid for. It follows ui::Engine::onFocus()
 * rather than checking per event, because the focus also moves under a mouse press this
 * class never sees, and a box clicked into and typed into in one frame would otherwise lose
 * its first character.
 *
 * The cursor stays the app's. ui::Cursor takes points rather than events and a press has to
 * interleave with whatever else an app does with one - the editor offers the ui a press and
 * drives a camera with the one the ui did not take - so a seam that consumed mouse events
 * here would decide that for every app.
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
