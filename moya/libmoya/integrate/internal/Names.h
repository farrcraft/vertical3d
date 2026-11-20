/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/output/Cryptomatte.h>
#include <moya/libmoya/scene/Scene.h>

#include <string>
#include <vector>

namespace v3d::moya::integrate::internal {

/**
 * The names a camera sample's record names its object, asset or material by, by the index the
 * record holds. An object's is the innermost instance's name, by its index in the scene. An
 * asset's is the name of the group the outermost instance places, or of that instance when it
 * places a geometry. A material's is the name of the scene's material, by its place among the
 * prototypes' materials. An index of no object is an empty name.
 **/
std::vector<std::string> namesOf(output::Cryptomatte::Names names, const scene::Scene & scene, const geometry::Prototypes & prototypes);

};  // namespace v3d::moya::integrate::internal
