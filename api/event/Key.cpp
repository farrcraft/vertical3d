/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Key.h"

namespace v3d::event {
    /**
     **/
    Key::Key(const std::string& name, const boost::shared_ptr<Context>& context, bool pressed) noexcept :
        Event(name, context),
        pressed_(pressed) {
        EventData keyState = pressed;
        data(keyState);
    }

    /**
    **/
    bool Key::pressed() const noexcept {
        return pressed_;
    }

};  // namespace v3d::event
