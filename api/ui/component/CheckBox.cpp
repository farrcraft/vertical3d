/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CheckBox.h"

#include <string>

namespace v3d::ui::component {

CheckBox::CheckBox() :
    Component(Type::CheckBox),
    checked_(false) {
}

CheckBox::CheckBox(Type type) :
    Component(type),
    checked_(false) {
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
    // stamped here rather than by whoever built it, the way Button and MenuItem do it: an
    // app applying ADR-0017's destination guard drops anything that is not marked, so a
    // command that is not stamped is a command that never arrives
    event_.type(v3d::event::Type::Destination);
}

v3d::event::Event CheckBox::event() const {
    return event_;
}

};  // namespace v3d::ui::component
