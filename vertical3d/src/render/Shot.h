/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/rib/Handler.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Scene.h>

#include <iosfwd>

#include "RIBExportVisitor.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::render {

/**
 * The camera a render of the scene is seen through: the one scene camera that is selected, or
 * the view when there is not exactly one.
 *
 * @param view the active view's camera, whose picture size and aspect a scene camera takes
 **/
v3d::type::camera::Profile shotFrom(const scene::Scene& scene, const v3d::type::camera::Profile& view);

/**
 * The camera a scene camera is, with the placement and the lens an evaluation of its scene
 * found.
 **/
v3d::type::camera::Profile shotFrom(const scene::Evaluation& evaluation, const boost::shared_ptr<scene::item::Camera>& camera,
    const v3d::type::camera::Profile& view);

/**
 * Send the scene to a RIB handler, as an evaluation of it found it, seen from a camera, as the
 * output's settings render it. An item renders do not show is left out.
 *
 * The camera takes the picture's size and frame aspect from the settings, so the picture is
 * not stretched whatever the shape of the view it came from.
 **/
void shoot(v3d::render::offline::rib::Handler* handler, const scene::Scene& scene, const scene::Evaluation& evaluation,
    const v3d::type::camera::Profile& from, const RIBExportVisitor::Output& output);

/**
 * Write the shot shoot() sends as a RIB file, with the structure comments that open one.
 **/
void writeShot(std::ostream* stream, const scene::Scene& scene, const scene::Evaluation& evaluation,
    const v3d::type::camera::Profile& from, const RIBExportVisitor::Output& output);

};  // namespace v3d::editor::render
