/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Keyboard.h"

#include <api/input/Keyboard.h>
#include <api/render/realtime/Window.h>
#include <api/ui/Component.h>
#include <api/ui/Engine.h>
#include <api/ui/component/Type.h>

#include <SDL3/SDL.h>

#include <string>
#include <string_view>

namespace v3d::ui::shell {

/**
 **/
Keyboard::Keyboard(const boost::shared_ptr<Engine>& ui, const boost::shared_ptr<entt::dispatcher>& dispatcher,
    const boost::shared_ptr<v3d::render::realtime::Window>& window) :
    ui_(ui),
    window_(window),
    keys_(ui, dispatcher, clipboard()) {
    if (ui_) {
        ui_->onFocus([this](const boost::shared_ptr<Component>& focused) { follow(focused); });
    }
    // whatever the window was left composing, the focus is what says now
    follow(ui_ ? ui_->focused() : boost::shared_ptr<Component>());
}

/**
 **/
Keyboard::~Keyboard() {
    // the engine holds a callback that captured this, and it outlives this whenever an app
    // keeps the ui and rebuilds the seam
    if (ui_) {
        ui_->onFocus(Engine::Focused());
    }
}

/**
 **/
input::Keys& Keyboard::keys() noexcept {
    return keys_;
}

/**
 **/
input::Keys::Clipboard Keyboard::clipboard() {
    input::Keys::Clipboard board;
    board.read = []() -> std::string {
        char* held = SDL_GetClipboardText();
        if (held == nullptr) {
            return std::string();
        }
        const std::string text(held);
        SDL_free(held);
        return text;
    };
    board.write = [](std::string_view text) {
        // SDL takes a C string and a view is not one, so the run is copied to terminate it
        SDL_SetClipboardText(std::string(text).c_str());
    };
    return board;
}

/**
 **/
void Keyboard::follow(const boost::shared_ptr<Component>& focused) {
    if (!window_) {
        return;
    }
    const bool typed = focused && focused->type() == component::Type::TextBox;
    if (typed == window_->textInput()) {
        return;
    }
    window_->textInput(typed);
}

/**
 **/
bool Keyboard::event(const SDL_Event& event) {
    switch (event.type) {
    case SDL_EVENT_TEXT_INPUT:
        return keys_.text(event.text.text);
    case SDL_EVENT_KEY_DOWN: {
        // one table, shared with the device that binds the same key - api/input's
        const std::string name = v3d::input::keyName(event.key.key);
        if (name.empty()) {
            return false;  // a key the api has no name for is nothing the ui could act on
        }
        // the modifiers come off the event rather than from the keyboard's held state,
        // because what a key meant is what was down as it arrived
        const bool shifted = (event.key.mod & SDL_KMOD_SHIFT) != 0;
        const bool controlled = (event.key.mod & SDL_KMOD_CTRL) != 0;
        return keys_.press(name, shifted, controlled);
    }
    default:
        // a release is never taken: a key held as a box took the focus still has to be seen
        // to come up, or api/input holds it down for the rest of the run
        return false;
    }
}

};  // namespace v3d::ui::shell
