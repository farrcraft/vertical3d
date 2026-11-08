/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/view/Tracing.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

/** Pictures of what rays hit, which light nothing. Host code, which launches kernel::view. **/
namespace v3d::moya::view {

/** The hit view's channels: depth, normal, instance, face and coverage. **/
inline constexpr const char * kDepth = "depth";
inline constexpr const char * kNormal = "normal";
inline constexpr const char * kInstance = "instance";
inline constexpr const char * kFace = "face";
inline constexpr const char * kCoverage = "coverage";

/**
 * The hit view of a scene through its camera: one ray through the centre of each pixel and of the
 * lens, traced through the scene's prototypes.
 *
 * A pixel holds its ray's depth, the geometric normal it hit in world space, the index of its
 * innermost instance, the face of its mesh, and a coverage of one. A pixel whose ray met nothing
 * holds an infinite depth, a zero normal, an instance and a face of -1, and no coverage. The
 * beauty is black.
 *
 * @param time where the rays are drawn over the shutter, from 0 at its opening to 1 at its close
 * @param tracer the traversal the rays are traced through
 * @throws std::runtime_error when the hierarchy is asked for and the CPU device's library cannot be
 *         loaded
 **/
film::Snapshot hits(device::CpuDevice & device, const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger,
    float time = 0.0F, Tracer tracer = Tracer::Hierarchy);

/**
 * Writes a hit view to an EXR file of float channels, the beauty and each of the hit view's
 * channels. The beauty is named as ACEScg, as the film holds it.
 *
 * @param path the file, as UTF-8
 * @throws std::exception, from OpenEXR, when the file cannot be written
 **/
void writeHits(const std::string & path, const film::Snapshot & snapshot);

};  // namespace v3d::moya::view
