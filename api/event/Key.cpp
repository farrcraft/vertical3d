/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Key.h"

namespace v3d::event {
    /**
     **/
    Key::Key(const std::string& name, bool pressed) noexcept :
        Event(name, "keyboard", pressed ? "down" : "up"),
        pressed_(pressed) {
    }

    /**
    **/
    bool Key::pressed() const noexcept {
        return pressed_;
    }

};  // namespace v3d::event
