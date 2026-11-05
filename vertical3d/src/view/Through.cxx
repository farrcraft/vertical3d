/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Through.h"

#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/CreateItem.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <cmath>
#include <optional>

#include "ViewPort.h"

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::view {

/**
 **/
Framed framed(const scene::item::Camera::Lens& lens, const glm::mat4& world, const v3d::type::camera::Profile& view, const glm::uvec2& picture) {
    Framed made{ .profile = scene::item::profile_of(lens, world, view) };
    const glm::vec2 size(view.size());
    if (size.x <= 0.0F || size.y <= 0.0F || picture.x == 0 || picture.y == 0) {
        made.frame = glm::vec4(0.0F, 0.0F, size.x, size.y);
        return made;
    }
    const float aspect = static_cast<float>(picture.x) / static_cast<float>(picture.y);
    glm::vec2 frame(size.y * aspect, size.y);
    if (frame.x > size.x) {
        frame = glm::vec2(size.x, size.x / aspect);
    }
    made.frame = glm::vec4((size - frame) * 0.5F, frame);

    // the lens spans the frame's height, so the view's height spans that much more
    const float stretch = size.y / frame.y;
    made.profile.fov(glm::degrees(2.0F * std::atan(std::tan(glm::radians(lens.fov) * 0.5F) * stretch)));
    made.profile.orthoZoom(lens.extent * stretch);
    return made;
}

/**
 **/
boost::shared_ptr<scene::item::Camera> lookedThrough(const scene::Scene& scene, const ViewPort::Through& through) {
    if (through.camera != 0) {
        for (const boost::shared_ptr<scene::item::Item>& item : scene.items()) {
            if (item->documentId() == through.camera) {
                return scene::item::as<scene::item::Camera>(item);
            }
        }
        return nullptr;
    }
    if (!through.scene) {
        return nullptr;
    }
    for (const boost::shared_ptr<scene::item::Item>& item : scene.selection()) {
        if (const boost::shared_ptr<scene::item::Camera> camera = scene::item::as<scene::item::Camera>(item)) {
            return camera;
        }
    }
    for (const boost::shared_ptr<scene::item::Item>& item : scene.items()) {
        if (const boost::shared_ptr<scene::item::Camera> camera = scene::item::as<scene::item::Camera>(item)) {
            return camera;
        }
    }
    return nullptr;
}

/**
 **/
void follow(const scene::Scene& scene, ViewPort* view) {
    if (view == nullptr || view->free()) {
        return;
    }
    const boost::shared_ptr<scene::item::Camera> camera = lookedThrough(scene, view->through());
    if (!camera) {
        // a camera that has gone leaves the view free where it was
        if (view->through().camera != 0) {
            view->letGo();
        }
        view->frame(std::nullopt);
        view->looking(0);
        return;
    }
    const scene::Evaluation evaluation = scene.evaluate();
    Framed shown = framed(evaluation.pose(*camera).lens, evaluation.placed(camera->id()).world, view->camera()->profile(), scene.renderSettings().size);
    shown.profile.name(camera->name());
    view->look(shown.profile);
    view->frame(shown.frame);
    view->looking(camera->documentId());
}

};  // namespace v3d::editor::view
