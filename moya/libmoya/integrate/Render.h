/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Profile.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

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

/** Where a render keeps its checkpoints, and when it stops before its last pass. **/
struct Checkpoints {
    /** The checkpoint written, as UTF-8, or empty to write none. **/
    std::string path;

    /**
     * The least time between two writes, in seconds. At zero the checkpoint is written after every
     * pass. The pass that ends a render is written whatever the time.
     **/
    double interval { 0.0 };

    /** Whether the render continues from the checkpoint at the path, in place of starting. **/
    bool resume { false };

    /**
     * Asked before each pass with the passes the film holds. A render it answers true for stops
     * there, writes its checkpoint, and returns what it holds. Empty asks nothing.
     **/
    std::function<bool(std::uint32_t)> stop;
};

/**
 * The scene's fingerprint, which a checkpoint holds: a 64-bit FNV-1a hash of the scene's
 * description, the film's channels and kSamplesPerPass. A scene changed by less than the
 * description's six digits keeps its fingerprint.
 **/
std::uint64_t fingerprint(const scene::Scene & scene, const std::vector<film::Channel> & channels);

/**
 * A scene rendered through the integrator on the CPU, through the hierarchy. Each pixel takes the
 * samples the options name, rounded up to a power of two, in passes of at most kSamplesPerPass.
 * While the options' noiseThreshold is above zero, the film stops the pixels whose noise falls
 * below it, checked before each pass once the pixels hold minSamples, and a render whose pixels
 * have all stopped ends.
 * The pixel filter and its width, the frame that seeds the sampler, and the camera are the
 * options'. The picture is the same on any thread count and whatever the passes hold.
 *
 * A render that resumes reads the checkpoint, and continues at the pass after the last it holds.
 * Its picture is the same, byte for byte, as that of a render that never stopped.
 *
 * A render given a profile fills it. A counted render gives the same picture as one that is not.
 *
 * @throws std::runtime_error when the CPU device's library cannot be loaded, or from OpenColorIO
 *         when a colour names a space the configuration cannot convert into ACEScg
 * @throws output::Refused when the checkpoint to resume from is not one of this scene's
 * @throws std::exception, from OpenEXR or the file system, when a checkpoint cannot be read or
 *         written
 **/
film::Snapshot render(device::CpuDevice & device, const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger,
    const colour::Config & config, const Checkpoints & checkpoints = {}, Profile * profile = nullptr);

};  // namespace v3d::moya::integrate
