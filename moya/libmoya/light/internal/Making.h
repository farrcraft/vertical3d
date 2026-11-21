/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/ShapedLight.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/light/internal/Profiles.h>
#include <moya/libmoya/light/internal/Textures.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstdint>

namespace v3d::moya::light::internal {

/**
 * A light of the scene, made from its object alone, before it is placed among the others, with
 * the handle of its object.
 *
 * light holds what it emits, its shape and its cone. Its image and its profile are places among
 * the textures and the profiles it was made with. size is what its power is measured by: a shaped
 * light's area in the world weighed by its texture's average, a point's area, a dome's integral of
 * its image, and nothing for a distant light. read is the image its texture gave, which a dome of
 * a layout moya does not read holds without emitting by it. shaping is the mean its profile
 * multiplies its emission by, or one with none. normalise says whether a mesh light's emission is
 * divided by its area.
 **/
struct Made {
    scene::Handle handle;
    kernel::light::Light light;
    float size { 0.0F };
    std::uint32_t read { kernel::geometry::kNoPrimitive };
    float shaping { 1.0F };
    bool normalise { false };
};

/** A distant light's size, as UsdLux measures it from the half angle of its cone. **/
float coneFactor(float half);

/** Where a light with no shape stands over the shutter, as an instance would place it. **/
geometry::Instance placement(const scene::Scene & scene, const scene::Handle & handle);

/** A light with a shape, where its instance places it as the shutter opens. **/
Made shapedLight(const scene::Scene & scene, const geometry::Prototypes & prototypes, const geometry::ShapedLight & shaped, const colour::Config & config,
    Textures & textures, Profiles & profiles);

/** A sphere light that is a point, where it stands as the shutter opens. **/
Made pointLight(const scene::Scene & scene, const scene::Handle & handle, float open, const colour::Config & config, Profiles & profiles);

/** A distant light. **/
Made distantLight(const scene::Scene & scene, const scene::Handle & handle, const colour::Config & config);

/** A dome light. **/
Made domeLight(const scene::Scene & scene, const scene::Handle & handle, const colour::Config & config, Textures & textures);

/** What a mesh light emits, before the geometry it names places it. **/
Made meshLight(const scene::Scene & scene, const scene::Handle & handle, const colour::Config & config);

};  // namespace v3d::moya::light::internal
