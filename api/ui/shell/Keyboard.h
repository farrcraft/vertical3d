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
 * The platform side of input::Keys. It turns an SDL event into a press() or a text() call
 * on input::Keys, and an app's Engine::onEvent() passes it every event.
 *
 * It decodes the event, reads the modifiers from it, supplies the clipboard and turns the
 * platform's text input on and off. input::Keys does none of these itself.
 *
 * A key the ui takes returns true. The app returns that from Engine::onEvent(), so the input
 * engine does not also map the key to its bound command.
 * Only a key going down is ever taken. A release always goes through, so a key held when a
 * box took the focus is still seen to come up and nothing is left stuck down.
 *
 * **Text input follows the focus.** The platform composes no text until text input is on,
 * so this turns it on while a text box holds the keyboard and off again after. It listens
 * through ui::Engine::onFocus() instead of checking on each event, because the focus also
 * moves on a mouse press, which this class never sees.
 *
 * The app handles the cursor itself. input::Cursor takes points, not events, so an app
 * passes a press to it directly.
 **/
class Keyboard final {
 public:
    /**
     * @param ui where the focus is held, and what reports it moving
     * @param dispatcher where a focused component's event is sent
     * @param window whose text input is turned on as the focus reaches a text box, and off
     *        as it leaves. Without a window the keys still edit, but no characters arrive
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
     * The input::Keys underneath, for an app with a key that did not come from the event
     * loop, or for a test driving the ui.
     **/
    input::Keys& keys() noexcept;

    /**
     * The platform's clipboard, which a text box's cut, copy and paste read and write.
     * Public so that an app can pass it to any other input::Keys it builds.
     **/
    static input::Keys::Clipboard clipboard();

 private:
    /**
     * Turn text input on while the focused component takes typed text, and off otherwise.
     * Called with the newly focused component.
     **/
    void follow(const boost::shared_ptr<Component>& focused);

    boost::shared_ptr<Engine> ui_;
    boost::shared_ptr<v3d::render::realtime::Window> window_;
    input::Keys keys_;
};

};  // namespace v3d::ui::shell
