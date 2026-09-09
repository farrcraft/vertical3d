/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "ReyesPrimitive.h"

namespace v3d::moya {

ReyesPrimitive::ReyesPrimitive() {
}

ReyesPrimitive::~ReyesPrimitive() {
}

bool ReyesPrimitive::diceable(void) const {
    return diceable_;
}

v3d::type::geometry::AABBox ReyesPrimitive::bound(void) const {
    return v3d::type::geometry::AABBox();
}

void ReyesPrimitive::split(RenderContext & rc) {
}

bool ReyesPrimitive::dice(boost::shared_ptr<MicroPolygonGrid> & /* grid */, RenderContext & /* rc */) {
    return false;
}

void ReyesPrimitive::diceable(bool status) {
    diceable_ = status;
}

bool ReyesPrimitive::placed(void) const {
    return placed_;
}

void ReyesPrimitive::place(const glm::mat4x4 & toEye, const glm::vec3 & color, const glm::vec3 & normal) {
    placement_ = toEye;
    color_ = color;
    normal_ = normal;
    placed_ = true;
}

const glm::mat4x4 & ReyesPrimitive::placement(void) const {
    return placement_;
}

const glm::vec3 & ReyesPrimitive::color(void) const {
    return color_;
}

const glm::vec3 & ReyesPrimitive::normal(void) const {
    return normal_;
}

};  // namespace v3d::moya
