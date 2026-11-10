/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/light/Lights.h>
#include <moya/libmoya/scene/Scene.h>

#include <boost/shared_ptr.hpp>

/** The scene's lights, made ready for the integrator. Host code. **/
namespace v3d::moya::light {

/**
 * The lights with a shape that the prototypes place, in a device's memory as the integrator
 * reads them. Each light's radiance is UsdLux's: its intensity times two to its exposure, times
 * its colour as an illuminant, converted into ACEScg from the space it names. With a colour
 * temperature, the colour tints a Planckian radiator at that temperature of a luminance of one.
 * With normalize, the radiance is divided by the light's area in the world, as it stands when
 * the shutter opens, so its power is the same at any size. A light of no area is not divided.
 *
 * A rect or disk light emits from the face its -z axis points to, and a sphere or cylinder light
 * from all of it. A cone angle is held to [0, 180] degrees and a softness to [0, 1].
 *
 * It reads the device's memory, so the device outlives it.
 **/
class Lights final {
 public:
    /**
     * @throws std::runtime_error, from OpenColorIO, when a light's colour names a space the
     *         configuration cannot convert into ACEScg
     **/
    Lights(device::Device & device, const scene::Scene & scene, const geometry::Prototypes & prototypes, const colour::Config & config);

    /** The lights, as kernel code reads them. **/
    kernel::light::Lights view() const;

 private:
    boost::shared_ptr<device::Buffer> lights_;
    boost::shared_ptr<device::Buffer> byId_;
};

};  // namespace v3d::moya::light
