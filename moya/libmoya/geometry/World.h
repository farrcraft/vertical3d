/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
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

    /** What was left out and why, once each, as the log reports it. **/
    std::vector<std::string> reports;
};

/**
 * The triangles of every mesh the scene's instances place, in world space.
 *
 * Each instance in the world places its prototype, and an instance of a group places every
 * instance in the group, inside its own transformation. Each chain of instances that reaches a
 * mesh is one placement, in the order of the instances' indices.
 *
 * Some chains are left out, and each is reported once:
 * - a group that holds itself, directly or through another group
 * - a chain deeper than kernel::geometry::kInstanceDepth
 * - a prototype of a kind not yet traced; meshes alone are traced
 *
 * An instance whose prototype was removed places nothing. One whose group was removed is in no
 * group, and so is placed by nothing. A transformation is its first sample.
 **/
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

    /** How many triangles are placed. **/
    std::uint32_t triangles() const;

 private:
    boost::shared_ptr<device::Buffer> points_;
    boost::shared_ptr<device::Buffer> faces_;
    boost::shared_ptr<device::Buffer> placements_;
};

};  // namespace v3d::moya::geometry
