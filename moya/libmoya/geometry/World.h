/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/geometry/Flat.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/BruteForce.h>
#include <moya/libmoya/kernel/geometry/Placed.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstdint>

#include <boost/shared_ptr.hpp>

#include <glm/mat4x4.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

namespace v3d::moya::geometry {

/**
 * Every primitive of every geometry the prototypes' instances place: triangles in world space, and
 * points and quadrics in their own space with the transformation into it.
 *
 * Each instance in the world places its prototype, and an instance of a group places every
 * instance in the group, inside its own transformation. Each chain of instances that reaches a
 * geometry is one placement, in the order of the instances' indices, visible to the rays every
 * instance of the chain is. A chain deeper than kernel::geometry::kInstanceDepth is left out, as
 * gather() reports. The reports are the prototypes'.
 *
 * Each transformation and each geometry is placed as it stands at a time, which is held to the
 * shutter. With no time, they are placed as they stand when the shutter opens. A point's radius
 * is half its width.
 **/
Flat flatten(const Prototypes & prototypes, float time);
Flat flatten(const Prototypes & prototypes);

/** A time held to the prototypes' shutter: the opening before it opens, and the closing after. **/
float heldTime(const Prototypes & prototypes, float time);

/** An instance's transformation at a time, interpolated between its keys when it moves. **/
glm::mat4 transformAt(const Instance & instance, float time);

/** Every primitive the scene's instances place: flatten() of what gather() gives. **/
Flat flatten(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger);

/**
 * The placed triangles of a scene in a device's memory. The world reads that memory, so the
 * device outlives it.
 **/
class World final {
 public:
    World(device::Device & device, const Flat & flat);

    /** The placed triangles, as kernel code reads them. **/
    kernel::geometry::Placed view() const;

    /** The placement each triangle belongs to, as kernel code reads it. **/
    kernel::Array<std::uint32_t> runs() const;

    /** How many triangles are placed. **/
    std::uint32_t triangles() const;

 private:
    boost::shared_ptr<device::Buffer> points_;
    boost::shared_ptr<device::Buffer> faces_;
    boost::shared_ptr<device::Buffer> placements_;
    boost::shared_ptr<device::Buffer> runs_;
    boost::shared_ptr<device::Buffer> shapes_;
    boost::shared_ptr<device::Buffer> spheres_;
    boost::shared_ptr<device::Buffer> quadrics_;
};

};  // namespace v3d::moya::geometry
