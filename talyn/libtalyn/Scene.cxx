/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scene.h"

#include <vector>

#include <glm/geometric.hpp>

namespace v3d::talyn {

namespace {

/**
 * The plane of a triangle, wound the way its corners are. A triangle with no area names no
 * plane and answers zero rather than a normalised nothing.
 **/
glm::vec3 plane(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c) {
    const glm::vec3 across = glm::cross(b - a, c - a);
    const float area = glm::length(across);
    return area > 1.0e-8f ? across / area : glm::vec3(0.0f);
}

};  // namespace

Triangle::Triangle(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c, const glm::vec3 & colour) :
    a_(a), b_(b), c_(c), colour_(colour), geometric_(plane(a, b, c)) {
    na_ = geometric_;
    nb_ = geometric_;
    nc_ = geometric_;
}

Triangle::Triangle(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c, const glm::vec3 & colour,
    const glm::vec3 & na, const glm::vec3 & nb, const glm::vec3 & nc) :
    a_(a), b_(b), c_(c), colour_(colour), na_(na), nb_(nb), nc_(nc), geometric_(plane(a, b, c)) {
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

const glm::vec3 & Triangle::geometricNormal() const {
    return geometric_;
}

glm::vec3 Triangle::shadingNormal(float u, float v) const {
    const glm::vec3 normal = na_ * (1.0f - u - v) + nb_ * u + nc_ * v;
    // interpolating unit normals does not give a unit one back, and three corners whose
    // normals cancel give none at all - the plane is what is left to answer with
    const float length = glm::length(normal);
    return length > 0.0f ? normal / length : geometric_;
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
