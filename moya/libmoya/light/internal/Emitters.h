/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/light/Shading.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstdint>
#include <functional>
#include <map>
#include <vector>

namespace v3d::moya::light::internal {

/**
 * Calls found with every chain of instances from a group's members that places a geometry, by
 * the instances' places among the prototypes', outermost first, and the geometry's place among
 * the prototypes'. A chain deeper than kernel::geometry::kInstanceDepth places nothing, as a
 * traversal leaves it out.
 **/
void chains(const geometry::Prototypes & prototypes, const std::vector<std::uint32_t> & members, std::vector<std::uint32_t> & chain,
    const std::function<void(const std::vector<std::uint32_t> &, std::uint32_t)> & found);

/**
 * The material a chain of instances gives its geometry: the innermost instance's that names one,
 * or kNoMaterial.
 **/
std::uint32_t materialOf(const geometry::Prototypes & prototypes, const std::vector<std::uint32_t> & placed);

/**
 * The light an emitting material makes: OpenPBR's emission luminance in its emission colour,
 * which camera rays see, in ACEScg. It is an object, not a light of the scene. It emits from both
 * faces where the instance that places its geometry is double-sided. A material that is not
 * layered, or emits no luminance, makes none.
 **/
bool materialEmitter(const std::vector<kernel::surface::Material> & materials, std::uint32_t material, bool doubleSided, kernel::light::Light & out);

/**
 * The mesh light that makes each geometry of the scene emit, by the geometry's index in the
 * scene: the first, in the order of their indices, of those that name it.
 **/
std::map<std::uint32_t, scene::Handle> meshEmitters(const scene::Scene & scene);

/**
 * Everything the lights of the prototypes' geometries are made from, as bytes. That is each chain
 * of instances from the world whose geometry a mesh light names, whose material emits, or whose
 * material encloses a medium. It holds the instances' transformations, what the material emits
 * and the geometry's points or dimensions. Two prototypes give the same bytes when they give their
 * geometries the same lights. Their volumes are left out.
 **/
std::vector<std::uint8_t> emitting(const geometry::Prototypes & prototypes, const std::vector<kernel::surface::Material> & materials,
    const std::map<std::uint32_t, scene::Handle> & named, const Shading & shading);

};  // namespace v3d::moya::light::internal
