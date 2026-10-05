/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Primitive.h"

namespace v3d::render::offline::trace {

Primitive::Primitive(const glm::vec3 & colour) : colour_(colour) {
}

const glm::vec3 & Primitive::colour() const {
    return colour_;
}

int Primitive::motion() const {
    return motion_;
}

const v3d::render::offline::sl::Placed & Primitive::surface() const {
    return surface_;
}

void Primitive::surface(const v3d::render::offline::sl::Placed & shader) {
    surface_ = shader;
}

const glm::vec3 & Primitive::opacity() const {
    return opacity_;
}

const Lights & Primitive::lights() const {
    return lights_;
}

void Primitive::lights(const Lights & lit) {
    lights_ = lit;
}

void Primitive::opacity(const glm::vec3 & value) {
    opacity_ = value;
}

};  // namespace v3d::render::offline::trace
