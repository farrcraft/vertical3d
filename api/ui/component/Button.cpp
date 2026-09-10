/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Button.h"

#include <string>

namespace v3d::ui::component {

Button::Button() :
    Component(component::Type::Button),
    state_(STATE_NORMAL),
    toggle_(false),
    checked_(false) {
    // a control exists to be driven, so it asks for the press and the focus that a panel
    // laid over a scene must not take - ADR-0034 and ADR-0040
    pickable(true);
    focusable(true);
}

void Button::label(const std::string& str) {
    label_ = str;
}

std::string_view Button::label() const {
    return label_;
}

Button::ButtonState Button::state() const {
    return state_;
}

void Button::state(ButtonState s) {
    state_ = s;
}

void Button::event(const v3d::event::Event& destination) {
    event_ = destination;
    event_.type(v3d::event::Type::Destination);
}

v3d::event::Event Button::event() const {
    return event_;
}

void Button::icon(const std::string& source) {
    icon_ = source;
}

std::string_view Button::icon() const {
    return icon_;
}

v3d::render::realtime::TextureHandle Button::texture() const noexcept {
    return texture_;
}

void Button::texture(const v3d::render::realtime::TextureHandle& tex) noexcept {
    texture_ = tex;
}

void Button::toggle(bool on) {
    toggle_ = on;
}

bool Button::toggle() const {
    return toggle_;
}

void Button::checked(bool on) {
    checked_ = on;
}

bool Button::checked() const {
    return toggle_ && checked_;
}

};  // namespace v3d::ui::component
