/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scene.h"

#include <vector>

namespace v3d::talyn {

Triangle::Triangle(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c, const glm::vec3 & colour) :
    a_(a), b_(b), c_(c), colour_(colour) {
}

const glm::vec3 & Triangle::a() const {
    return a_;
}

const glm::vec3 & Triangle::b() const {
    return b_;
}

const glm::vec3 & Triangle::c() const {
    return c_;
}

const glm::vec3 & Triangle::colour() const {
    return colour_;
}

Scene::Scene() {
}

v3d::type::Camera & Scene::camera() {
    return camera_;
}

const v3d::type::Camera & Scene::camera() const {
    return camera_;
}

void Scene::add(const Triangle & triangle) {
    triangles_.push_back(triangle);
}

const std::vector<Triangle> & Scene::triangles() const {
    return triangles_;
}

const glm::vec3 & Scene::background() const {
    return background_;
}

void Scene::background(const glm::vec3 & colour) {
    background_ = colour;
}

};  // namespace v3d::talyn
