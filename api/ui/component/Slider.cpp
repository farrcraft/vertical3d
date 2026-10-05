/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Slider.h"

#include <algorithm>
#include <cmath>

#include "Type.h"

namespace v3d::ui::component {

/**
 **/
Slider::Slider() :
    Component(Type::Slider),
    minimum_(0.0f),
    maximum_(1.0f),
    step_(0.0f),
    value_(0.0f) {
    // pickable for its drag and focusable for its keys, which a plain component is not
    pickable(true);
    focusable(true);
}

/**
 **/
void Slider::range(float minimum, float maximum, float step) {
    minimum_ = std::min(minimum, maximum);
    maximum_ = std::max(minimum, maximum);
    step_ = std::max(step, 0.0f);
    value_ = snapped(value_);
}

/**
 **/
float Slider::minimum() const noexcept {
    return minimum_;
}

/**
 **/
float Slider::maximum() const noexcept {
    return maximum_;
}

/**
 **/
float Slider::step() const noexcept {
    return step_;
}

/**
 **/
bool Slider::value(float set) {
    const float was = value_;
    value_ = snapped(set);
    return value_ != was;
}

/**
 **/
float Slider::value() const noexcept {
    return value_;
}

/**
 **/
float Slider::fraction() const noexcept {
    const float span = maximum_ - minimum_;
    return span > 0.0f ? (value_ - minimum_) / span : 0.0f;
}

/**
 **/
bool Slider::drag(const glm::vec2& point) {
    if (size().x <= 0.0f) {
        // never drawn, so there is no track to read a point against
        return false;
    }
    // the thumb is a square as tall as the track, and its centre runs from half of it in from
    // one end to half of it in from the other, so that is the range a point is read against:
    // a press on the thumb's centre lands on the value that drew it there
    const float side = size().y;
    const float travel = size().x - side;
    const float along = travel > 0.0f
        ? std::clamp((point.x - position().x - side * 0.5f) / travel, 0.0f, 1.0f)
        : std::clamp((point.x - position().x) / size().x, 0.0f, 1.0f);
    return value(minimum_ + along * (maximum_ - minimum_));
}

/**
 **/
void Slider::event(const v3d::event::Event& destination) {
    event_ = destination;
    event_.type(v3d::event::Type::Destination);
}

/**
 **/
v3d::event::Event Slider::event() const {
    return event_;
}

/**
 **/
float Slider::snapped(float set) const noexcept {
    // the ends are always reachable, even where the steps do not divide the range and the
    // last one is short
    if (set >= maximum_) {
        return maximum_;
    }
    float clamped = std::max(set, minimum_);
    if (step_ > 0.0f) {
        clamped = std::min(minimum_ + std::round((clamped - minimum_) / step_) * step_, maximum_);
    }
    return clamped;
}

};  // namespace v3d::ui::component
