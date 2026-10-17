/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Primitive.h"

namespace v3d::moya::reyes {

Primitive::Primitive() {
}

Primitive::~Primitive() {
}

bool Primitive::diceable(void) const {
    return diceable_;
}

v3d::type::geometry::AABBox Primitive::bound(void) const {
    return v3d::type::geometry::AABBox();
}

void Primitive::split(RenderContext & rc) {
}

bool Primitive::dice(boost::shared_ptr<MicroPolygonGrid> & /* grid */, RenderContext & /* rc */) {
    return false;
}

void Primitive::diceable(bool status) {
    diceable_ = status;
}

bool Primitive::placed(void) const {
    return placed_;
}

void Primitive::place(const glm::mat4x4 & toEye, const glm::vec3 & color,
    const glm::vec3 & normal, const Shading & shading) {
    placement_ = toEye;
    color_ = color;
    normal_ = normal;
    shading_ = shading;
    placed_ = true;
}

const glm::mat4x4 & Primitive::placement(void) const {
    return placement_;
}

void Primitive::motion(const v3d::render::offline::MovingTransform & toEye) {
    motion_ = toEye;
}

const v3d::render::offline::MovingTransform & Primitive::motion(void) const {
    return motion_;
}

const glm::vec3 & Primitive::color(void) const {
    return color_;
}

const glm::vec3 & Primitive::normal(void) const {
    return normal_;
}

const Shading & Primitive::shading(void) const {
    return shading_;
}

};  // namespace v3d::moya::reyes
