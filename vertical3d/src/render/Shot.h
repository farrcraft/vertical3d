/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/rib/Handler.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/RIBExportVisitor.h>
#include <vertical3d/src/scene/Scene.h>

#include <iosfwd>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * The camera a render of the scene is seen through: the one scene camera that is selected, or
 * the view when there is not exactly one.
 *
 * @param view the active view's camera, whose picture size and aspect a scene camera takes
 **/
v3d::type::camera::Profile shotFrom(const Scene& scene, const v3d::type::camera::Profile& view);

/**
 * The camera a scene camera is, with the placement and the lens an evaluation of its scene
 * found.
 **/
v3d::type::camera::Profile shotFrom(const Evaluation& evaluation, const boost::shared_ptr<Camera>& camera,
    const v3d::type::camera::Profile& view);

/**
 * Send the scene to a RIB handler, as an evaluation of it found it, seen from a camera, as the
 * output's settings render it.
 *
 * The camera takes the picture's size and frame aspect from the settings, so the picture is
 * not stretched whatever the shape of the view it came from.
 **/
void shoot(v3d::render::offline::rib::Handler* handler, const Scene& scene, const Evaluation& evaluation,
    const v3d::type::camera::Profile& from, const RIBExportVisitor::Output& output);

/**
 * Write the shot shoot() sends as a RIB file, with the structure comments that open one.
 **/
void writeShot(std::ostream* stream, const Scene& scene, const Evaluation& evaluation,
    const v3d::type::camera::Profile& from, const RIBExportVisitor::Output& output);

};  // namespace v3d::editor
