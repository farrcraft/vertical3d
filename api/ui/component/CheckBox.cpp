/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CheckBox.h"

#include <string>

namespace v3d::ui::component {

CheckBox::CheckBox() :
    Component(Type::CHECKBOX),
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
}

v3d::event::Event CheckBox::event() const {
    return event_;
}

};  // namespace v3d::ui::component
