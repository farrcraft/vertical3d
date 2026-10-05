/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scene.h"

#include <limits>
#include <vector>

#include <boost/make_shared.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

namespace v3d::render::offline::trace {

Scene::Scene() {
}

void Scene::view(const glm::mat4x4 & toCamera) {
    view_ = toCamera;
}

glm::mat4x4 Scene::view() const {
    return view_;
}

glm::vec3 Scene::eye() const {
    return glm::vec3(glm::inverse(view_)[3]);
}

int Scene::motion(const v3d::render::offline::MovingTransform & placed) {
    if (!placed.moving()) {
        return -1;
    }
    // the pieces of one primitive share a motion, and so do the next primitive's when
    // nothing has been placed in between
    if (motions_.empty() || motions_.back().open() != placed.open() || motions_.back().close() != placed.close() ||
        motions_.back().times() != placed.times()) {
        motions_.push_back(placed);
    }
    return static_cast<int>(motions_.size()) - 1;
}

void Scene::add(const boost::shared_ptr<Primitive> & primitive, const v3d::render::offline::MovingTransform & placed) {
    primitive->motion_ = motion(placed);
    primitives_.push_back(primitive);
}

void Scene::add(const Triangle & triangle) {
    primitives_.push_back(boost::make_shared<Triangle>(triangle));
}

void Scene::add(const Triangle & triangle, const v3d::render::offline::MovingTransform & placed) {
    add(boost::make_shared<Triangle>(triangle), placed);
}

void Scene::add(const Sphere & sphere, const v3d::render::offline::MovingTransform & placed) {
    add(boost::make_shared<Sphere>(sphere), placed);
}

const std::vector<boost::shared_ptr<const Primitive>> & Scene::primitives() const {
    return primitives_;
}

void Scene::add(const v3d::render::offline::sl::Placed & light) {
    lights_.push_back(light);
}

const std::vector<v3d::render::offline::sl::Placed> & Scene::lights() const {
    return lights_;
}

Scene::Poses Scene::poses(float time) const {
    Poses poses;
    poses.ahead.resize(motions_.size());
    poses.backward.resize(motions_.size());
    for (std::size_t i = 0; i < motions_.size(); i++) {
        poses.ahead[i] = motions_[i].at(time) * glm::inverse(motions_[i].reference());
        poses.backward[i] = glm::inverse(poses.ahead[i]);
    }
    return poses;
}

bool Scene::nearest(const v3d::type::geometry::Ray & ray, float from, Hit* hit, float time) const {
    return nearest(ray, from, hit, poses(time));
}

bool Scene::nearest(const v3d::type::geometry::Ray & ray, float from, Hit* hit, const Poses & poses) const {
    const Primitive* met = nullptr;
    Intersection nearest;
    nearest.distance = std::numeric_limits<float>::max();
    for (const boost::shared_ptr<const Primitive> & primitive : primitives_) {
        const int motion = primitive->motion();
        Pose pose;
        if (motion >= 0) {
            pose.ahead = &poses.ahead[motion];
            pose.backward = &poses.backward[motion];
        }
        Intersection found;
        if (primitive->intersect(ray, from, pose, &found) && found.distance < nearest.distance) {
            nearest = found;
            met = primitive.get();
        }
    }
    if (met == nullptr) {
        return false;
    }
    if (hit == nullptr) {
        return true;
    }

    hit->primitive = met;
    hit->distance = nearest.distance;
    hit->point = ray.origin() + ray.direction() * nearest.distance;
    hit->colour = met->colour();
    met->describe(nearest, hit);
    // a moving primitive's normals are carried forward to the time its hit was found at
    const int motion = met->motion();
    if (motion >= 0) {
        const glm::mat3 normals = glm::transpose(glm::inverse(glm::mat3(poses.ahead[motion])));
        hit->normal = glm::normalize(normals * hit->normal);
        if (glm::length(hit->geometric) > 0.0f) {
            hit->geometric = glm::normalize(normals * hit->geometric);
        }
    }
    hit->incident = ray.direction();
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

};  // namespace v3d::render::offline::trace
