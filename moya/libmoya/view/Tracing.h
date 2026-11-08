/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Traversal.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/kernel/geometry/BruteForce.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>

#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::view {

/** Which traversal rays are traced through. **/
enum class Tracer : std::uint8_t {
    /** The hierarchy, through the CPU device's library for its instruction set. **/
    Hierarchy,

    /** The brute-force traversal, which every other is checked against. **/
    BruteForce
};

/**
 * A scene's prototypes on a CPU device: their triangles placed in world space, and the hierarchy
 * of each prototype when the hierarchy is traced through. It reads the device's memory, so the
 * device outlives it.
 **/
class Tracing final {
 public:
    Tracing(device::CpuDevice & device, const geometry::Prototypes & prototypes, Tracer tracer);

    Tracer tracer() const;
    const geometry::World & world() const;

    /** The brute-force traversal over the placed triangles. **/
    kernel::geometry::BruteForce bruteForce() const;

    /** The CPU's traversal over the hierarchies. Empty unless the hierarchy is traced through. **/
    device::cpu::Traversal hierarchy() const;

 private:
    Tracer tracer_;
    geometry::World world_;
    boost::shared_ptr<device::Buffer> nodes_;
    boost::shared_ptr<device::Buffer> references_;
    boost::shared_ptr<device::Buffer> points_;
    boost::shared_ptr<device::Buffer> instances_;
    std::uint32_t root_ { 0 };
};

/** Each ray's nearest hit, and whether anything lies on it. **/
struct Traced {
    std::vector<kernel::geometry::Hit> nearest;
    std::vector<bool> any;
};

/**
 * Traces each of a list of rays through a scene's prototypes.
 *
 * @throws std::runtime_error when the hierarchy is asked for and the CPU device's library cannot be
 *         loaded
 **/
Traced trace(device::CpuDevice & device, const geometry::Prototypes & prototypes, const std::vector<kernel::geometry::Ray> & rays,
    Tracer tracer);

};  // namespace v3d::moya::view
