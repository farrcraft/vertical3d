/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstdint>

#include <boost/shared_ptr.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

/** Rendering a scene through the new core's integrator. Host code, which launches its kernel. **/
namespace v3d::moya::integrate {

/** The most samples one pass adds to each pixel. **/
inline constexpr std::uint32_t kSamplesPerPass = 16;

/**
 * The integrator's settings from a scene's options: its depth limits and how it finds the
 * lights. A depth below zero is taken as zero.
 **/
kernel::integrate::Settings settingsOf(const scene::Scene & scene);

/**
 * A scene rendered through the integrator on the CPU, through the hierarchy. Each pixel takes the
 * samples the options name, rounded up to a power of two, in passes of at most kSamplesPerPass.
 * The pixel filter and its width, the frame that seeds the sampler, and the camera are the
 * options'. The picture is the same on any thread count and whatever the passes hold.
 *
 * @throws std::runtime_error when the CPU device's library cannot be loaded, or from OpenColorIO
 *         when a colour names a space the configuration cannot convert into ACEScg
 **/
film::Snapshot render(device::CpuDevice & device, const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger,
    const colour::Config & config);

};  // namespace v3d::moya::integrate
