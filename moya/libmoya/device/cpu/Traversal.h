/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/cpu/Geometry.h>
#include <moya/libmoya/device/cpu/Instance.h>
#include <moya/libmoya/device/cpu/MotionNode.h>
#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/geometry/primitive/Quadric.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Ray.h>

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::device::cpu {

/**
 * The CPU's traversal: a hierarchy of eight-wide nodes for each mesh and each group, whose nodes
 * are tested eight children at a time with SIMD. Still hierarchies are held in one array of
 * nodes and moving ones in another, and a ray starts at the world's.
 *
 * A ray's time is held to the shutter. Each moving transformation and each moving geometry are
 * interpolated at that time, and a moving node's boxes too.
 *
 * A ray enters an instance only when the instance is visible to its kind and the chain it is
 * inside has room for one more. A hit's chain is the instances it entered, outermost first.
 *
 * Only the CPU's libraries define nearest() and any(), each for its own instruction set. libmoya
 * fills the arrays and passes the traversal to a library's entry point, and never calls either.
 **/
struct Traversal {
    kernel::Array<Node> nodes;
    kernel::Array<MotionNode> motionNodes;

    /**
     * The hierarchies' references. A geometry's leaf holds the index of a primitive within the
     * geometry, and a group's the index of an instance.
     **/
    kernel::Array<std::uint32_t> references;

    /** Three points per triangle, or a point's centre, at each sample, in their own space. **/
    kernel::Array<glm::vec3> points;

    /** Every point's width, at each sample. **/
    kernel::Array<float> widths;

    /** Every quadric's dimensions, at each sample. **/
    kernel::Array<kernel::geometry::primitive::Quadric> quadrics;

    /** The times of the samples of everything that moves. **/
    kernel::Array<float> times;

    kernel::Array<Geometry> geometries;

    /** The keys of every moving instance. **/
    kernel::Array<kernel::geometry::Key> keys;

    kernel::Array<Instance> instances;

    /** The root of the world's hierarchy, whose leaves hold the instances in no group. **/
    std::uint32_t world { 0 };

    /** One when the world's hierarchy is of moving nodes. **/
    std::uint32_t worldMoving { 0 };

    /** The times the shutter opens and closes. **/
    float open { 0.0F };
    float close { 0.0F };

    kernel::geometry::Hit nearest(const kernel::geometry::Ray & ray) const;
    bool any(const kernel::geometry::Ray & ray) const;
};

};  // namespace v3d::moya::device::cpu
