/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cstdint>

namespace v3d::moya::integrate::internal {

/**
 * A parameter of the scene's options, or its fallback while the scene has none.
 *
 * @throws std::invalid_argument when the options have no such parameter
 **/
const scene::Value & option(const scene::Scene & scene, const char * name);

/** An integer option, held to zero or more. **/
std::uint32_t count(const scene::Scene & scene, const char * name);

};  // namespace v3d::moya::integrate::internal
