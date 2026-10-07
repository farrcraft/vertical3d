/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CheckBox.h"

#include <string>

namespace v3d::ui::component {

CheckBox::CheckBox() :
    CheckBox(Type::CheckBox) {
}

CheckBox::CheckBox(Type type) :
    Component(type),
    checked_(false) {
    // a control is pickable and focusable from the start; a plain component is neither, so a
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
    // marked as a command here rather than by the caller, as Button and MenuItem do, because a
    // listener that accepts only Destination events drops an unmarked one
    event_.type(v3d::event::Type::Destination);
}

v3d::event::Event CheckBox::event() const {
    return event_;
}

};  // namespace v3d::ui::component
