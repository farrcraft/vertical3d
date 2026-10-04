/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scene.h"

#include <limits>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

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

void Scene::add(const Triangle & triangle, const v3d::render::offline::MovingTransform & placed) {
    if (!placed.moving()) {
        add(triangle);
        return;
    }
    // the triangles of one primitive share a motion, and so do the next primitive's when
    // nothing has been placed in between
    if (motions_.empty() || motions_.back().open() != placed.open() || motions_.back().close() != placed.close() ||
        motions_.back().times() != placed.times()) {
        motions_.push_back(placed);
    }
    Triangle moving(triangle);
    moving.motion_ = static_cast<int>(motions_.size()) - 1;
    triangles_.push_back(moving);
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

bool Scene::nearest(const v3d::type::geometry::Ray & ray, float from, Hit* hit, float time) const {
    // where each motion has carried its triangles by this time from where they are stored,
    // and the way back
    std::vector<glm::mat4x4> ahead(motions_.size());
    std::vector<glm::mat4x4> backward(motions_.size());
    for (std::size_t i = 0; i < motions_.size(); i++) {
        ahead[i] = motions_[i].at(time) * glm::inverse(motions_[i].open());
        backward[i] = glm::inverse(ahead[i]);
    }

    float closest = std::numeric_limits<float>::max();
    const Triangle* found = nullptr;
    float bestU = 0.0f;
    float bestV = 0.0f;
    for (const Triangle & triangle : triangles_) {
        const int motion = triangle.motion();
        const v3d::type::geometry::Ray local = motion < 0 ? ray :
            v3d::type::geometry::Ray(glm::vec3(backward[motion] * glm::vec4(ray.origin(), 1.0f)),
                glm::mat3(backward[motion]) * ray.direction());
        float distance = 0.0f;
        float u = 0.0f;
        float v = 0.0f;
        if (!local.intersects(triangle.a(), triangle.b(), triangle.c(), &distance, &u, &v)) {
            continue;
        }
        if (motion >= 0) {
            // a distance along the ray taken back is in the stored pose's units, which a
            // motion that scales does not keep
            const glm::vec3 there(ahead[motion] * glm::vec4(local.origin() + local.direction() * distance, 1.0f));
            distance = glm::dot(there - ray.origin(), ray.direction());
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
    if (found->motion() >= 0) {
        const glm::mat3 normals = glm::transpose(glm::inverse(glm::mat3(ahead[found->motion()])));
        hit->normal = glm::normalize(normals * hit->normal);
        if (glm::length(hit->geometric) > 0.0f) {
            hit->geometric = glm::normalize(normals * hit->geometric);
        }
    }
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

unsigned int Scene::traceDepth() const {
    return traceDepth_;
}

void Scene::traceDepth(unsigned int depth) {
    traceDepth_ = depth;
}

int Triangle::motion() const {
    return motion_;
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
