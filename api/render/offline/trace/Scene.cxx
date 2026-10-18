/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scene.h"

#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/sl/Placed.h>
#include <api/type/geometry/AABBox.h>
#include <api/type/geometry/Ray.h>

#include "Hierarchy.h"
#include "Hit.h"
#include "Intersection.h"
#include "Pose.h"
#include "Primitive.h"
#include "Sphere.h"
#include "Triangle.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <vector>

#include <boost/make_shared.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

namespace v3d::render::offline::trace {

namespace {

/** How many steps a moving primitive's corners are followed through across its motion. **/
constexpr int SWEEP_STEPS = 16;

Hierarchy::Box still(const v3d::type::geometry::AABBox & stored) {
    Hierarchy::Box box;
    box.extend(stored.min());
    box.extend(stored.max());
    return box;
}

/**
 * A box that holds a moving primitive at every time of its motion. The primitive at a time is
 * its stored box's corners through that time's pose, so the box follows those corners across
 * the motion. A rotation carries a corner along an arc between two steps, and the arc stays
 * within one step's length of either end. The box is grown by the longest step to hold it.
 **/
Hierarchy::Box swept(const v3d::type::geometry::AABBox & stored, const v3d::render::offline::MovingTransform & motion) {
    glm::vec3 corners[8];
    stored.vertices(corners);
    const glm::mat4x4 back = glm::inverse(motion.reference());
    Hierarchy::Box box;
    glm::vec3 previous[8];
    float longest = 0.0F;
    for (int step = 0; step <= SWEEP_STEPS; step++) {
        const float u = static_cast<float>(step) / static_cast<float>(SWEEP_STEPS);
        const float time = motion.times().x + (u * (motion.times().y - motion.times().x));
        const glm::mat4x4 ahead = motion.at(time) * back;
        for (std::size_t k = 0; k < 8; k++) {
            const glm::vec3 placed(ahead * glm::vec4(corners[k], 1.0F));
            box.extend(placed);
            if (step > 0) {
                longest = std::max(longest, glm::length(placed - previous[k]));
            }
            previous[k] = placed;
        }
    }
    box.min -= glm::vec3(longest);
    box.max += glm::vec3(longest);
    return box;
}

};  // namespace

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

void Scene::projection(const Projection & projected) {
    projection_ = projected;
}

const Scene::Projection & Scene::projection() const {
    return projection_;
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
    built_ = false;
}

void Scene::add(const Triangle & triangle) {
    primitives_.push_back(boost::make_shared<Triangle>(triangle));
    built_ = false;
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

void Scene::build() const {
    std::vector<Hierarchy::Box> boxes;
    std::vector<std::size_t> bounded;
    unbounded_.clear();
    for (std::size_t i = 0; i < primitives_.size(); i++) {
        v3d::type::geometry::AABBox stored;
        if (!primitives_[i]->bound(&stored)) {
            unbounded_.push_back(i);
            continue;
        }
        const int motion = primitives_[i]->motion();
        boxes.push_back(motion >= 0 ? swept(stored, motions_[static_cast<std::size_t>(motion)]) : still(stored));
        bounded.push_back(i);
    }
    hierarchy_.build(boxes);
    bounded_ = bounded;
    built_ = true;
}

bool Scene::nearest(const v3d::type::geometry::Ray & ray, float from, Hit* hit, const Poses & poses) const {
    if (!built_) {
        build();
    }
    const Primitive* met = nullptr;
    std::size_t metIndex = std::numeric_limits<std::size_t>::max();
    Intersection nearest;
    nearest.distance = std::numeric_limits<float>::max();
    // the hierarchy visits primitives in no fixed order, so a tie goes to the one added first
    const auto test = [&](std::size_t index) {
        const Primitive & primitive = *primitives_[index];
        const int motion = primitive.motion();
        Pose pose;
        if (motion >= 0) {
            pose.ahead = &poses.ahead[static_cast<std::size_t>(motion)];
            pose.backward = &poses.backward[static_cast<std::size_t>(motion)];
        }
        Intersection found;
        if (primitive.intersect(ray, from, pose, &found) &&
            (found.distance < nearest.distance || (found.distance == nearest.distance && index < metIndex))) {
            nearest = found;
            met = &primitive;
            metIndex = index;
        }
    };
    for (const std::size_t index : unbounded_) {
        test(index);
    }
    hierarchy_.walk(ray, from, &nearest.distance, [&](std::size_t item) { test(bounded_[item]); });
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
        if (glm::length(hit->geometric) > 0.0F) {
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
