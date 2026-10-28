/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Delegate.h"

#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <cstdint>

#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 **/
Delegate::Delegate() {
}

/**
 **/
Delegate::~Delegate() {
}

/**
 **/
void Delegate::render(const Scene& scene, const v3d::type::camera::Profile& from, const RenderSettings& settings) {
    cancel();
    last_ = Signature::of(scene, from, settings);
    rendered_ = true;
    start(scene, from, settings);
}

/**
 **/
bool Delegate::update(const Scene& scene, const v3d::type::camera::Profile& from, const RenderSettings& settings) {
    if (!rendered_ || Signature::of(scene, from, settings) == last_) {
        return false;
    }
    render(scene, from, settings);
    return true;
}

/**
 **/
Delegate::Signature Delegate::Signature::of(const Scene& scene, const v3d::type::camera::Profile& from,
    const RenderSettings& settings) {
    Signature signature;
    const Evaluation evaluation = scene.evaluate();
    for (const boost::shared_ptr<Item>& item : scene.items()) {
        const boost::shared_ptr<Item> parent = scene.parent(item->id());
        signature.items.emplace_back(item->documentId(), item->revision(), parent ? parent->documentId() : 0);
        signature.poses.push_back(values(item->kind(), evaluation.pose(*item), glm::vec3(0.0F)));
    }
    signature.eye = from.eye();
    signature.direction = from.direction();
    signature.up = from.up();
    signature.orthographic = from.orthographic();
    signature.fov = from.fov();
    signature.zoom = from.orthoZoom();
    signature.clipping = from.clipping();
    signature.settings = settings;
    return signature;
}

};  // namespace v3d::editor
