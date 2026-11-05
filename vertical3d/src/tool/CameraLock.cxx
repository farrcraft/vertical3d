/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CameraLock.h"

#include <api/type/Transform.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/command/Edit.h>
#include <vertical3d/src/command/Keyed.h>
#include <vertical3d/src/command/Stack.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/CreateItem.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/view/Through.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cmath>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>

namespace v3d::editor::tool {

/**
 **/
CameraLock::CameraLock(const boost::shared_ptr<command::Stack>& history) :
    history_(history) {
}

/**
 **/
command::Edit::Values CameraLock::placed(const scene::Scene& scene, const scene::item::Camera& camera, const view::ViewPort& view) {
    command::Edit::Values values = command::Edit::of(camera);
    const v3d::type::camera::Profile& profile = view.camera()->profile();

    // where the view is in the world, measured in the space of the camera's parent
    const glm::mat4 world = glm::translate(glm::mat4(1.0F), profile.eye()) * glm::mat4_cast(scene::item::facing(profile));
    const scene::Evaluation evaluation = scene.evaluate();
    const boost::shared_ptr<scene::item::Item> parent = scene.parent(camera.id());
    const glm::mat4 under = parent ? evaluation.placed(parent->id()).world : glm::mat4(1.0F);
    const v3d::type::Transform local = scene::placement(glm::inverse(under) * world);
    values.pose.placement.position = local.position;
    values.pose.placement.rotation = local.rotation;

    // the view sees past the frame, so its lens is wider than the camera's by as much
    const float height = static_cast<float>(profile.size().y);
    const float stretch = view.frame() && view.frame()->w > 0.0F && height > 0.0F ? height / view.frame()->w : 1.0F;
    values.pose.lens.fov = glm::degrees(2.0F * std::atan(std::tan(glm::radians(profile.fov()) * 0.5F) / stretch));
    values.pose.lens.extent = profile.orthoZoom() / stretch;
    return values;
}

/**
 **/
void CameraLock::moved(const boost::shared_ptr<scene::Scene>& scene, const view::ViewPort& view) {
    if (!scene || !view.through().locked) {
        return;
    }
    const boost::shared_ptr<scene::item::Camera> camera = view::lookedThrough(*scene, view.through());
    if (!camera) {
        return;
    }
    if (pending_ && pending_->camera != camera) {
        ended();
    }
    if (!pending_) {
        pending_ = Pending{ .scene = scene, .camera = camera, .before = command::Edit::of(*camera) };
    }
    const command::Edit::Values wanted = placed(*scene, *camera, view);
    if (wanted.pose.lens.valid()) {
        command::Edit::apply(camera.get(), wanted);
    }
}

/**
 **/
void CameraLock::ended() {
    if (!pending_) {
        return;
    }
    const Pending done = *pending_;
    pending_.reset();
    if (command::Edit::same(done.before, command::Edit::of(*done.camera)) || !history_) {
        return;
    }
    const command::Keyed::Change change{ .item = done.camera, .before = done.before.pose, .after = scene::pose(*done.camera) };
    history_->push(command::Keyed::record(done.scene, boost::make_shared<command::Edit>(done.camera, done.before, "move camera"), { change }));
}

};  // namespace v3d::editor::tool
