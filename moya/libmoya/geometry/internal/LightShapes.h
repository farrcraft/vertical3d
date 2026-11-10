/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <vector>

namespace v3d::moya::geometry::internal {

/** Every rect, disk, sphere and cylinder light of a scene, in the order of their indices. **/
std::vector<scene::Handle> shapedLights(const scene::Scene & scene);

/**
 * The instance in the world that places a light's shape, as gather() says, with no prototype yet.
 * A cylinder's quadric lies along z, so a cylinder light's transformation turns it onto x at each
 * of its keys.
 **/
Instance lightInstance(const scene::Scene & scene, const scene::Handle & handle);

/** A light's shape in its own space, with its values appended to the prototypes. **/
Geometry lightShape(const scene::Scene & scene, const scene::Handle & handle, Prototypes & prototypes);

};  // namespace v3d::moya::geometry::internal
