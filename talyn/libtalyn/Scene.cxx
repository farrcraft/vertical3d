/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scene.h"

#include <limits>
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

v3d::type::camera::Camera & Scene::camera() {
    return camera_;
}

const v3d::type::camera::Camera & Scene::camera() const {
    return camera_;
}

void Scene::add(const Triangle & triangle) {
    triangles_.push_back(triangle);
}

const std::vector<Triangle> & Scene::triangles() const {
    return triangles_;
}

void Scene::add(const v3d::render::offline::sl::Placed & light) {
    lights_.push_back(light);
}

const std::vector<v3d::render::offline::sl::Placed> & Scene::lights() const {
    return lights_;
}

bool Scene::nearest(const v3d::type::geometry::Ray & ray, float from, Hit* hit) const {
    float closest = std::numeric_limits<float>::max();
    const Triangle* found = nullptr;
    float bestU = 0.0f;
    float bestV = 0.0f;
    for (const Triangle & triangle : triangles_) {
        float distance = 0.0f;
        float u = 0.0f;
        float v = 0.0f;
        if (!ray.intersects(triangle.a(), triangle.b(), triangle.c(), &distance, &u, &v)) {
            continue;
        }
        if (distance <= from || distance >= closest) {
            continue;
        }
        closest = distance;
        found = &triangle;
        bestU = u;
        bestV = v;
    }
    if (found == nullptr || hit == nullptr) {
        return found != nullptr;
    }
    hit->triangle = found;
    hit->distance = closest;
    hit->point = ray.origin() + ray.direction() * closest;
    hit->normal = found->shadingNormal(bestU, bestV);
    hit->geometric = found->geometricNormal();
    hit->incident = ray.direction();
    hit->u = bestU;
    hit->v = bestV;
    return true;
}

const glm::vec3 & Scene::background() const {
    return background_;
}

void Scene::background(const glm::vec3 & colour) {
    background_ = colour;
}

const v3d::render::offline::sl::Placed & Triangle::surface() const {
    return surface_;
}

void Triangle::surface(const v3d::render::offline::sl::Placed & shader) {
    surface_ = shader;
}

const glm::vec3 & Triangle::opacity() const {
    return opacity_;
}

void Triangle::opacity(const glm::vec3 & value) {
    opacity_ = value;
}

};  // namespace v3d::talyn
