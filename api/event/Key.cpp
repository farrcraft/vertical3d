/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Key.h"

namespace v3d::event {

    /**
     **/
    Key::Key(const std::string_view& name, bool pressed) noexcept :
        name_(name),
        pressed_(pressed) {
    }

    /**
    **/
    bool Key::pressed() const noexcept {
        return pressed_;
    }

    /**
    **/
    std::string_view Key::name() const noexcept {
        return name_;
    }

};  // namespace v3d::event
