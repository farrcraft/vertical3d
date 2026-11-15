/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Group.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/ShapedLight.h>
#include <moya/libmoya/kernel/geometry/primitive/Quadric.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/Primvar.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/texture/Request.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

namespace v3d::moya::geometry {

class Cache;

/**
 * Every prototype the scene's instances reach, each in its own space, and the instances that place
 * them. A prototype placed many times is held once.
 *
 * A group comes after every group it holds, so a hierarchy built over the groups in order finds
 * each member's built first. The world is the group of every instance in no group.
 **/
struct Prototypes {
    /**
     * Every geometry's points, each in its geometry's own space: three per triangle, or one per
     * point's centre, at each sample.
     **/
    std::vector<glm::vec3> points;

    /** Every point's width, at each sample. **/
    std::vector<float> widths;

    /** Every quadric's dimensions, at each sample. **/
    std::vector<kernel::geometry::primitive::Quadric> quadrics;

    /** Every segment's control points, two or four to a segment, at each sample. **/
    std::vector<glm::vec4> controls;

    /** One for each segment that closes its curve, and zero for one the next carries on from. **/
    std::vector<std::uint32_t> ends;

    /** The times of the samples of everything that has more than one. **/
    std::vector<float> times;

    /** The face of its mesh each triangle was cut from, and the curve each segment was. **/
    std::vector<std::uint32_t> faces;

    /**
     * Every geometry's primvars that shading reads, as their first sample holds them, spread over
     * its primitives. A colour's values are as the scene gives them, in the space that
     * primvarSpaces names for it, which is empty for the working space. A primvar that is not a
     * colour has no space.
     **/
    std::vector<kernel::surface::Primvar> primvars;
    std::vector<float> primvarValues;
    std::vector<std::optional<std::string>> primvarSpaces;

    /** The name of each primvar shading reads, by the number a Primvar names it with. **/
    std::vector<std::string> primvarNames;

    /**
     * Every material an instance names, each held once, with its surface node's colour in the
     * space materialSpaces names for it.
     **/
    std::vector<kernel::surface::Material> materials;
    std::vector<std::string> materialSpaces;

    /** The scene's material object each of the materials was made from. **/
    std::vector<scene::Handle> materialHandles;

    /**
     * Every material's program: its instructions, and the values its registers start from, each
     * material's after the one before. Each colour constant is named by its first value, with the
     * space its values are in, which is empty for the working space.
     **/
    std::vector<kernel::program::Instruction> instructions;
    std::vector<float> programValues;
    std::vector<std::uint32_t> programColours;
    std::vector<std::string> programSpaces;

    /** Every image the programs read, by the number an Image instruction names it with. **/
    std::vector<texture::Request> images;

    std::vector<Geometry> geometries;
    std::vector<Group> groups;
    std::vector<Instance> instances;
    Group world;

    /** Every rect, disk, sphere and cylinder light, in the order of their indices. **/
    std::vector<ShapedLight> lights;

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
 * - an instance of a subdivision surface that OpenSubdiv cannot refine, for the reasons
 *   geometry::subdivision::Surface() refuses it.
 * - an instance of a mesh whose faces cannot be cut: a negative count of vertices, counts that do
 *   not add up to the indices, or an index outside the points.
 * - an instance of curves that cannot be cut into segments: a curve of fewer than two vertices,
 *   one whose vertices make no whole segments, or counts that do not add up to the points.
 *
 * An instance whose prototype was removed places nothing. One whose group was removed is in no
 * group, and so is placed by nothing. A transformation, a mesh's points, a point cloud's centres
 * and widths, and a quadric's dimensions keep every sample, at their own times. A quadric's
 * dimensions are sampled together at every time any of them is, and curves' points and widths
 * too. Curves that give widths for neither each vertex nor each varying value take their constant
 * width, and are reported. An instance is visible to camera,
 * shadow and indirect rays as its parameters of those names say. The shutter is the camera's.
 *
 * Each rect, disk, sphere and cylinder light places its shape through an instance in the world,
 * after every other, whose index in the scene is the light's. Its shape is UsdLux's, in the
 * light's own space. A rect is its width along x and height along y, facing -z. A disk is its
 * radius, facing -z. A sphere is its radius, and a cylinder its radius and length along x, with no
 * caps. Each is visible to indirect rays, to camera rays only while its camera parameter is 1,
 * and never to shadow rays.
 **/
Prototypes gather(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger);

/** The prototypes gather() gives, each geometry the cache holds placed from it, as Cache says. **/
Prototypes gather(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger, Cache & cache);

};  // namespace v3d::moya::geometry
