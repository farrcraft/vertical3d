/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CheckBox.h"

#include <api/event/Event.h>
#include <api/event/Type.h>
#include <api/ui/Component.h>

#include "Type.h"

#include <string>
#include <string_view>

namespace v3d::ui::component {

CheckBox::CheckBox() :
    CheckBox(Type::CheckBox) {
}

CheckBox::CheckBox(Type type) :
    Component(type),
    checked_(false) {
    // a control is pickable and focusable from the start. A plain component is neither, so a
    // panel laid over a scene lets presses through
    pickable(true);
    focusable(true);
}

void CheckBox::label(const std::string& str) {
    label_ = str;
}

std::string_view CheckBox::label() const {
    return label_;
}

void CheckBox::checked(bool on) {
    checked_ = on;
}

bool CheckBox::checked() const {
    return checked_;
}

void CheckBox::event(const v3d::event::Event& destination) {
    event_ = destination;
    // marked as a Destination event here, not by the caller, because a listener that accepts
    // only Destination events drops an unmarked one
    event_.type(v3d::event::Type::Destination);
}

v3d::event::Event CheckBox::event() const {
    return event_;
}

};  // namespace v3d::ui::component
