/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextInput.h"

#include <api/event/Context.h>
#include <api/event/Event.h>
#include <api/event/State.h>

#include <string>
#include <string_view>

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
