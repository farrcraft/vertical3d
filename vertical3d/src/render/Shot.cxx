/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shot.h"

#include <api/render/offline/rib/Handler.h>
#include <api/render/offline/rib/Writer.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/CreateItem.h>
#include <vertical3d/src/scene/RIBExportVisitor.h>
#include <vertical3d/src/scene/Scene.h>

#include <ostream>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
v3d::type::camera::Profile shotFrom(const Scene& scene, const v3d::type::camera::Profile& view) {
    const std::vector<boost::shared_ptr<Camera>> cameras = scene.selected<Camera>();
    return cameras.size() == 1 ? shotFrom(scene, cameras.front(), view) : view;
}

/**
 **/
v3d::type::camera::Profile shotFrom(const Scene& scene, const boost::shared_ptr<Camera>& camera,
    const v3d::type::camera::Profile& view) {
    if (!camera) {
        return view;
    }
    return profile_of(*camera, scene.evaluate().placed(camera->id()).world, view);
}

/**
 **/
void shoot(v3d::render::offline::rib::Handler* handler, const Scene& scene, const v3d::type::camera::Profile& from,
    const RIBExportVisitor::Output& output) {
    if (handler == nullptr) {
        return;
    }
    v3d::type::camera::Profile profile(from);
    profile.size(output.settings.size);
    profile.pixelAspect(static_cast<float>(output.settings.size.x) / static_cast<float>(output.settings.size.y));
    v3d::type::camera::Camera camera(profile);
    camera.createProjection();
    camera.createView();

    RIBExportVisitor visitor(handler);
    visitor.begin(camera, output);
    scene.accept(&visitor);
    visitor.end();
}

/**
 **/
void writeShot(std::ostream* stream, const Scene& scene, const v3d::type::camera::Profile& from,
    const RIBExportVisitor::Output& output) {
    if (stream == nullptr) {
        return;
    }
    *stream << "##RenderMan RIB-Structure 1.1" << "\n" << "##Creator Vertical3D" << "\n";
    v3d::render::offline::rib::Writer writer(stream);
    shoot(&writer, scene, from, output);
}

};  // namespace v3d::editor
