/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstdint>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

namespace v3d::moya::geometry {

/** What an instance places: a mesh, or a group of instances. **/
enum class Prototype : std::uint8_t {
    Mesh,
    Group
};

/**
 * A mesh's triangles: a run of the faces every mesh's triangles were cut from, and the points of
 * its triangles, three per triangle, at each of its samples.
 **/
struct Mesh {
    std::uint32_t first { 0 };
    std::uint32_t count { 0 };
    kernel::geometry::PointSamples points;

    /** Whether the mesh's points move over the shutter. **/
    bool moves() const {
        return points.samples > 1;
    }
};

/**
 * An instance that places a prototype. Its transformation takes the prototype's space to the
 * space of the group that holds the instance, and its visibility holds the kinds of ray it is
 * visible to.
 **/
struct Instance {
    /** The instance's index in the scene, which a hit's chain names it by. **/
    std::uint32_t id { 0 };

    Prototype kind { Prototype::Mesh };

    /** The prototype's index among the meshes or the groups. **/
    std::uint32_t prototype { 0 };

    /** The transformation at its first sample, which holds at every time when it has no keys. **/
    glm::mat4 transform { 1.0F };

    /** The transformation at each of its samples, when it has more than one. **/
    std::vector<kernel::geometry::Key> keys;

    std::uint32_t visibility { 0 };

    /** Whether the instance's transformation moves over the shutter. **/
    bool moves() const {
        return keys.size() > 1;
    }
};

/** The instances a group holds, each by its index among every instance. **/
struct Group {
    std::vector<std::uint32_t> instances;
};

/**
 * Every prototype the scene's instances reach, each in its own space, and the instances that place
 * them. A prototype placed many times is held once.
 *
 * A group comes after every group it holds, so a hierarchy built over the groups in order finds
 * each member's built first. The world is the group of every instance in no group.
 **/
struct Prototypes {
    /** Three points per triangle at each of its mesh's samples, each in its mesh's own space. **/
    std::vector<glm::vec3> points;

    /** The times of the samples of every mesh that has more than one. **/
    std::vector<float> times;

    /** The face of its mesh each triangle was cut from. **/
    std::vector<std::uint32_t> faces;

    std::vector<Mesh> meshes;
    std::vector<Group> groups;
    std::vector<Instance> instances;
    Group world;

    /** The times the camera's shutter opens and closes. A time is held between them. **/
    float open { 0.0F };
    float close { 0.0F };

    /** What was left out and why, once each, as the log reports it. **/
    std::vector<std::string> reports;
};

/**
 * The prototypes the scene's instances reach from the world, with their instances.
 *
 * An instance places its prototype. An instance of a group places every instance in the group,
 * inside its own transformation. Instances keep the order of their indices.
 *
 * Some instances are left out, and each reason is reported once:
 * - an instance of a group that holds the instance's own group, directly or through another group.
 *   A group that holds itself would otherwise have no bounds.
 * - an instance that would be deeper in a chain than kernel::geometry::kInstanceDepth. It is still
 *   held where a shorter chain reaches it, and a traversal leaves it out where it is too deep.
 * - an instance of a prototype of a kind not yet traced. Meshes alone are traced.
 *
 * An instance whose prototype was removed places nothing. One whose group was removed is in no
 * group, and so is placed by nothing. A transformation and a mesh's points keep every sample, at
 * their own times. An instance is visible to camera, shadow and indirect rays as its parameters of
 * those names say. The shutter is the camera's.
 **/
Prototypes gather(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger);

};  // namespace v3d::moya::geometry
