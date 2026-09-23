/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Device.h"
#include "KeyState.h"

#include <string>

namespace v3d::input {

/**
 * What this library calls a key - "escape", "arrow_left", "a" - which is the name a
 * binding in mappings.json names and the name ui::Keys is handed.
 *
 * Public because ui::shell::Keyboard names the keys it hands a text box from this table
 * too, so a binding and a text box always agree on what a key is called.
 *
 * @return the name, or an empty string for a key this library has none for
 **/
std::string keyName(SDL_Keycode key);

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
