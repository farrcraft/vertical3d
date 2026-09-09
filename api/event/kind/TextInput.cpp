/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextInput.h"

#include <string>

namespace v3d::event::kind {

/**
 **/
TextInput::TextInput(const std::string& text, const boost::shared_ptr<Context>& context) :
    Event("text", context),
    text_(text) {
    state(State::Pressed);
}

/**
 **/
std::string_view TextInput::text() const noexcept {
    return text_;
}

};  // namespace v3d::event::kind
