/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Device.h"
#include "KeyState.h"

#include <string>
#include <string_view>

namespace v3d::input {

/**
 * What this library calls a key, such as "escape", "arrow_left" or "a". A binding in
 * mappings.json uses this name, and ui::Keys receives it.
 *
 * Public because ui::shell::Keyboard also names the keys it passes to a text box from this
 * table, so a binding and a text box always agree on what a key is called.
 *
 * @return the name, or an empty string for a key this library has none for
 **/
std::string keyName(SDL_Keycode key);

/**
 * Whether a name is one that keyName() returns for some key. A binding document is checked
 * against this, so a misspelt key is reported rather than bound to nothing.
 **/
bool isKeyName(std::string_view name);

/**
 **/
class Keyboard final : public Device {
 public:
    /**
     * Inherit base constructor
     **/
    using Device::Device;

    /**
     * @return bool true if the event was handled
     **/
    bool handleEvent(const SDL_Event& event);

    /**
     **/
    void flush() override;

    /**
     * @return what is held and what changed edge this frame
     **/
    const KeyState& state() const;

 private:
    KeyState state_;
};
};  // namespace v3d::input
