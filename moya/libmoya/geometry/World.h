/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/BruteForce.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstdint>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

namespace v3d::moya::geometry {

/**
 * Every triangle of a scene placed in world space, in host memory, as kernel::geometry::Placed
 * holds it on a device.
 **/
struct Flat {
    std::vector<glm::vec3> points;
    std::vector<std::uint32_t> faces;
    std::vector<kernel::geometry::Placement> placements;

    /** The placement each triangle belongs to, one per triangle. **/
    std::vector<std::uint32_t> runs;

    /** What was left out and why, once each, as the log reports it. **/
    std::vector<std::string> reports;
};

/**
 * The triangles of every mesh the prototypes' instances place, in world space.
 *
 * Each instance in the world places its prototype, and an instance of a group places every
 * instance in the group, inside its own transformation. Each chain of instances that reaches a
 * mesh is one placement, in the order of the instances' indices, visible to the rays every
 * instance of the chain is. A chain deeper than kernel::geometry::kInstanceDepth is left out, as
 * gather() reports. The reports are the prototypes'.
 **/
Flat flatten(const Prototypes & prototypes);

/** The triangles of every mesh the scene's instances place: flatten() of what gather() gives. **/
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
};

};  // namespace v3d::moya::geometry
