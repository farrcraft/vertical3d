/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Curve.h"

#include <api/curve/Curves.h>

#include <cmath>
#include <cstdint>

#include "Item.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
bool Curve::Style::valid() const {
    return std::isfinite(width) && width > 0.0F;
}

/**
 **/
Curve::Curve(const boost::shared_ptr<v3d::curve::Curves>& geometry) :
    geometry_(geometry ? geometry : boost::make_shared<v3d::curve::Curves>()) {
}

/**
 **/
Item::Kind Curve::kind() const noexcept {
    return KIND;
}

/**
 **/
const boost::shared_ptr<v3d::curve::Curves>& Curve::geometry() const noexcept {
    return geometry_;
}

/**
 **/
const Curve::Style& Curve::style() const noexcept {
    return style_;
}

/**
 **/
void Curve::style(const Style& style) noexcept {
    style_ = style;
    changed();
}

/**
 **/
std::uint64_t Curve::revision() const noexcept {
    return Item::revision() + geometry_->changes();
}

};  // namespace v3d::editor
