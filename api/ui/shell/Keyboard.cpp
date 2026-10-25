/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Keyboard.h"

#include <api/input/Keyboard.h>
#include <api/render/realtime/Window.h>
#include <api/ui/Component.h>
#include <api/ui/Engine.h>
#include <api/ui/Immediate.h>
#include <api/ui/component/Type.h>

#include <SDL3/SDL.h>

#include <functional>
#include <string>
#include <string_view>

namespace v3d::ui::shell {

/**
 **/
Keyboard::Keyboard(const boost::shared_ptr<Engine>& ui, const boost::shared_ptr<entt::dispatcher>& dispatcher,
    const boost::shared_ptr<v3d::render::realtime::Window>& window) :
    ui_(ui),
    layer_(nullptr),
    window_(window),
    keys_(ui, dispatcher, clipboard()) {
    if (ui_) {
        ui_->onFocus([this](const boost::shared_ptr<Component>& focused) { follow(focused); });
    }
    // set text input to match whatever is focused now, whatever state the window was left in
    follow(ui_ ? ui_->focused() : boost::shared_ptr<Component>());
}

/**
 **/
Keyboard::~Keyboard() {
    // the engine holds a callback that captured this object. The engine outlives it when an
    // app keeps the ui and builds a new Keyboard
    if (ui_) {
        ui_->onFocus(Engine::Focused());
    }
    immediate(nullptr);
}

/**
 **/
void Keyboard::immediate(Immediate* layer) {
    if (layer_ != nullptr) {
        layer_->onTyping(std::function<void(bool)>());
    }
    layer_ = layer;
    if (layer_ != nullptr) {
        layer_->clipboard(clipboard());
        layer_->onTyping([this](bool) { follow(ui_ ? ui_->focused() : boost::shared_ptr<Component>()); });
    }
    follow(ui_ ? ui_->focused() : boost::shared_ptr<Component>());
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
    const bool typed = (focused && component::traits(focused->type()).text) || (layer_ != nullptr && layer_->typing());
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
        return keys_.text(event.text.text) || (layer_ != nullptr && layer_->typed(event.text.text));
    case SDL_EVENT_KEY_DOWN: {
        // api/input's key names, the same table the key bindings use
        const std::string name = v3d::input::keyName(event.key.key);
        if (name.empty()) {
            return false;  // a key the api has no name for is nothing the ui could act on
        }
        // the modifiers are read from the event, not from the keyboard's current state,
        // because what counts is what was held when the key went down
        const bool shifted = (event.key.mod & SDL_KMOD_SHIFT) != 0;
        const bool controlled = (event.key.mod & SDL_KMOD_CTRL) != 0;
        return keys_.press(name, shifted, controlled, event.key.repeat) ||
            (layer_ != nullptr && layer_->press(name, shifted, controlled));
    }
    default:
        // a release is never taken. A key held while a box took the focus must still be seen
        // to come up, or api/input treats it as held for the rest of the run
        return false;
    }
}

};  // namespace v3d::ui::shell
