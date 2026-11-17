/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/geometry/primitive/Curves.h>
#include <moya/libmoya/kernel/geometry/primitive/Quadric.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/kernel/surface/Footprint.h>
#include <moya/libmoya/kernel/surface/Geometry.h>
#include <moya/libmoya/kernel/surface/Instance.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/Primvar.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/TextureFootprint.h>

#include <cmath>
#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

/** The scene's surfaces as shading reads them, apart from any device's traversal. **/
namespace v3d::moya::kernel::surface {

/** The least a tangent may hold, squared, before it is taken as none. **/
inline constexpr float kLeastTangent = 1.0e-12F;

/** How far in `s` and `t` a point with a footprint of no area is shifted along each axis. **/
inline constexpr float kShiftStep = 1.0F / 1024.0F;

/**
 * Every geometry the scene's instances place, each in its own space at each of its samples, with
 * the instances that place them and the primvars shading reads. A hit names a chain of instances
 * and a primitive, and at() turns it into a shading point as the surface stood at the hit's time.
 **/
struct Surfaces {
    /** Three points per triangle, or a point's centre, at each sample, in their own space. **/
    Array<glm::vec3> points;

    /** Every point's width, at each sample. **/
    Array<float> widths;

    /** Every quadric's dimensions, at each sample. **/
    Array<geometry::primitive::Quadric> quadrics;

    /** Every segment's control points, two or four to a segment, at each sample. **/
    Array<glm::vec4> controls;

    /** One for each segment that closes its curve, and zero for one the next carries on from. **/
    Array<std::uint32_t> ends;

    /** The times of the samples of everything that moves. **/
    Array<float> times;

    /** The face of its mesh each triangle was cut from, and the curve each segment was. **/
    Array<std::uint32_t> faces;

    Array<Geometry> geometries;

    /** The keys of every moving instance. **/
    Array<geometry::Key> keys;

    Array<Instance> instances;

    /** Each instance's place among the instances, by its index in the scene, or kNoPrimitive. **/
    Array<std::uint32_t> byId;

    Array<Primvar> primvars;

    /** Every material an instance names. **/
    Array<Material> materials;

    /** Every material's program: its instructions, and the values its registers start from. **/
    Array<program::Instruction> instructions;
    Array<float> programValues;

    /** Every primvar's values. **/
    Array<float> values;

    /** The times the shutter opens and closes. **/
    float open { 0.0F };
    float close { 0.0F };

    /** One when an instance's material encloses a medium. **/
    std::uint32_t media { 0 };

    /** A time held to the shutter, as a traversal holds a ray's. **/
    MOYA_KERNEL float held(float time) const {
        if (!(time > open)) {
            return open;
        }
        return time > close ? close : time;
    }

    /** A geometry's primvar of a name, or none. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the geometry, then the name
    MOYA_KERNEL const Primvar * primvar(std::uint32_t placed, std::uint32_t name) const {
        const Geometry & shape = geometries[placed];
        for (std::uint32_t i = 0; i < shape.primvarCount; ++i) {
            const Primvar & candidate = primvars[shape.primvars + i];
            if (candidate.name == name) {
                return &candidate;
            }
        }
        return nullptr;
    }

    /**
     * A primvar's value at a primitive and the weights of its corners, into components floats.
     * False, leaving out untouched, for a primvar of another width or of a layout it cannot read.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the primitive, then the floats
    MOYA_KERNEL bool read(const Primvar & source, std::uint32_t primitive, const glm::vec3 & weights, float * out, std::uint32_t components) const {
        if (source.components != components) {
            return false;
        }
        if (source.perPrimitive == 0U || source.perPrimitive == 1U) {
            const std::uint32_t from = source.first + (source.perPrimitive * primitive * components);
            for (std::uint32_t component = 0; component < components; ++component) {
                out[component] = values[from + component];
            }
            return true;
        }
        if (source.perPrimitive != 3U) {
            return false;
        }
        for (std::uint32_t component = 0; component < components; ++component) {
            float sum = 0.0F;
            for (std::uint32_t corner = 0; corner < 3U; ++corner) {
                sum += weights[static_cast<glm::length_t>(corner)] * values[source.first + ((((primitive * 3U) + corner) * components) + component)];
            }
            out[component] = sum;
        }
        return true;
    }

    /** A shading point's primvar of a name, into components floats, or false where it has none. **/
    MOYA_KERNEL bool read(const ShadingPoint & point, std::uint32_t name, float * out, std::uint32_t components) const {
        if (!point.found()) {
            return false;
        }
        const Primvar * found = primvar(point.geometry, name);
        return found != nullptr && read(*found, point.primitive, point.weights, out, components);
    }

    /**
     * A hit as a shading point, as its surface stood at the hit's time held to the shutter. A hit
     * that found nothing, or names an instance or a geometry the surfaces do not hold, gives a
     * point that found nothing.
     **/
    MOYA_KERNEL ShadingPoint at(const geometry::Hit & hit, const geometry::Ray & ray) const {
        ShadingPoint out;
        std::uint32_t placed = kGroup;
        glm::mat4 toWorld { 1.0F };
        const float time = held(hit.time);
        std::uint32_t material = kNoMaterial;
        if (!placing(hit, time, toWorld, placed, material)) {
            return out;
        }
        out.material = material < materials.size ? material : kNoMaterial;
        const Geometry & shape = geometries[placed];
        const glm::mat4 toLocal = glm::inverse(toWorld);
        geometry::Ray local = ray;
        local.origin = glm::vec3(toLocal * glm::vec4(ray.origin, 1.0F));
        local.direction = glm::vec3(toLocal * glm::vec4(ray.direction, 0.0F));

        glm::vec3 point = local.at(hit.distance);
        glm::vec3 leave = point;
        glm::vec3 geometric { 0.0F, 0.0F, 1.0F };
        glm::vec3 shading { 0.0F };
        out.uv = hit.uv;
        out.face = hit.primitive;
        switch (shape.form) {
            case geometry::primitive::Form::Triangles:
                triangle(shape, placed, hit, time, out, point, leave, geometric, shading);
                out.face = faces[shape.faces + hit.primitive];
                break;
            case geometry::primitive::Form::RoundPoints:
                geometric = point - geometry::sampleAt(shape.points, points.data, times.data, hit.primitive, time);
                break;
            case geometry::primitive::Form::FlatPoints:
                geometric = -local.direction;
                break;
            case geometry::primitive::Form::Quadric:
                geometric = geometry::primitive::quadricNormal(geometry::primitive::sampledQuadric(shape.quadric, quadrics.data, times.data, time), point);
                out.face = 0U;
                break;
            case geometry::primitive::Form::RoundLinearCurves:
            case geometry::primitive::Form::FlatLinearCurves:
            case geometry::primitive::Form::RoundCubicCurves:
            case geometry::primitive::Form::FlatCubicCurves: {
                const std::uint32_t count = geometry::primitive::controlsOf(shape.form);
                glm::vec4 segment[4];
                for (std::uint32_t control = 0; control < count; ++control) {
                    segment[control] = geometry::sampleAt(shape.controls, controls.data, times.data, (hit.primitive * count) + control, time);
                }
                geometric = geometry::primitive::curveNormal(shape.form, segment, point, hit.uv.y, local.direction);
                out.face = faces[shape.faces + hit.primitive];
                break;
            }
        }

        // a normal moves by the inverse transpose of the transformation to the world
        const glm::mat3 normalToWorld = glm::transpose(glm::mat3(toLocal));
        out.position = glm::vec3(toWorld * glm::vec4(point, 1.0F));
        out.leave = glm::vec3(toWorld * glm::vec4(leave, 1.0F));
        out.geometric = glm::normalize(normalToWorld * geometric);
        const glm::vec3 turned = normalToWorld * shading;
        out.shading = glm::dot(turned, turned) > 0.0F ? glm::normalize(turned) : out.geometric;
        if (glm::dot(out.shading, out.geometric) < 0.0F) {
            out.shading = -out.shading;
        }
        out.frame = Frame::around(out.shading);
        out.front = glm::dot(ray.direction, out.geometric) < 0.0F;
        out.geometry = placed;
        out.primitive = hit.primitive;
        out.toWorld = toWorld;
        out.time = time;
        return out;
    }

    /**
     * The frame a shading point's tangent gives, around its shading normal. The tangent is the
     * geometry's `tangent` primvar where it has one, and otherwise the direction `s` increases
     * across a triangle whose corners each hold `st`. The bitangent is the normal's cross product
     * with the tangent, turned to the side `t` increases on. A point with neither, or where they
     * lie along the normal, keeps the frame it was given.
     **/
    MOYA_KERNEL Frame tangentFrame(const ShadingPoint & point) const {
        if (!point.found()) {
            return point.frame;
        }
        const glm::mat3 turn { point.toWorld };
        const glm::vec3 normal = point.shading;
        glm::vec3 along { 0.0F };
        glm::vec3 across { 0.0F };
        float given[3] {};
        if (read(point, kTangent, given, 3U)) {
            along = turn * glm::vec3 { given[0], given[1], given[2] };
        } else if (!textureDerivatives(point, along, across)) {
            return point.frame;
        }
        const glm::vec3 flat = along - (normal * glm::dot(normal, along));
        if (!(glm::dot(flat, flat) > kLeastTangent)) {
            return point.frame;
        }
        Frame out;
        out.normal = normal;
        out.tangent = glm::normalize(flat);
        out.bitangent = glm::cross(normal, out.tangent);
        if (glm::dot(out.bitangent, across) < 0.0F) {
            out.bitangent = -out.bitangent;
        }
        return out;
    }

    /**
     * A shading point's footprint in its texture coordinates: each axis as the change in `s` and
     * `t` along it, from how a triangle's point moves as `s` and `t` increase. A point with no
     * `st` that varies across its triangle gives a footprint of no area.
     **/
    MOYA_KERNEL TextureFootprint textureFootprint(const ShadingPoint & point) const {
        TextureFootprint out;
        glm::vec3 alongS { 0.0F };
        glm::vec3 alongT { 0.0F };
        if (!point.found() || !textureDerivatives(point, alongS, alongT)) {
            return out;
        }
        const float ss = glm::dot(alongS, alongS);
        const float st = glm::dot(alongS, alongT);
        const float tt = glm::dot(alongT, alongT);
        const float determinant = (ss * tt) - (st * st);
        if (!(std::fabs(determinant) > kLeastTangent)) {
            return out;
        }
        out.major = inTexture(point.footprint.major, alongS, alongT, ss, st, tt, determinant);
        out.minor = inTexture(point.footprint.minor, alongS, alongT, ss, st, tt, determinant);
        return out;
    }

    /**
     * A shading point moved by a step along an axis of its footprint, as a program's Shift moves
     * it: the first axis for one, the second for two, and not at all for any other. Each step is
     * the axis's whole length, so the point moves across the area its footprint covers. A point
     * whose footprint has no area steps kShiftStep along `s` for the first axis and along `t`
     * for the second, where it lies on a triangle whose `st` varies, and otherwise stays.
     *
     * The point's position moves by the step. On a triangle its corners' weights and its `uv`
     * move too, as the step's nearest sum of the triangle's edges says, so every primvar reads as
     * it would where the point moved to. Its normals, frame and footprint stay as they were.
     **/
    MOYA_KERNEL ShadingPoint shifted(const ShadingPoint & point, std::uint32_t axis) const {
        if (!point.found() || (axis != 1U && axis != 2U)) {
            return point;
        }
        glm::vec3 step = axis == 1U ? point.footprint.major : point.footprint.minor;
        if (!(point.footprint.width() > 0.0F) || !(glm::dot(step, step) > 0.0F)) {
            glm::vec3 alongS { 0.0F };
            glm::vec3 alongT { 0.0F };
            if (!textureDerivatives(point, alongS, alongT)) {
                return point;
            }
            step = (axis == 1U ? alongS : alongT) * kShiftStep;
        }
        ShadingPoint out = point;
        out.position += step;
        if (geometries[point.geometry].form != geometry::primitive::Form::Triangles) {
            return out;
        }
        glm::vec3 corner[3];
        corners(point.geometry, point.primitive, point.time, corner);
        const glm::mat3 turn { point.toWorld };
        const glm::vec3 edge1 = turn * (corner[1] - corner[0]);
        const glm::vec3 edge2 = turn * (corner[2] - corner[0]);
        const float e11 = glm::dot(edge1, edge1);
        const float e12 = glm::dot(edge1, edge2);
        const float e22 = glm::dot(edge2, edge2);
        const float determinant = (e11 * e22) - (e12 * e12);
        if (!(std::fabs(determinant) > kLeastTangent)) {
            return out;
        }
        const float onFirst = glm::dot(step, edge1);
        const float onSecond = glm::dot(step, edge2);
        const float first = ((onFirst * e22) - (onSecond * e12)) / determinant;
        const float second = ((onSecond * e11) - (onFirst * e12)) / determinant;
        out.weights += glm::vec3 { -first - second, first, second };
        out.uv += glm::vec2 { first, second };
        return out;
    }

    /**
     * A triangle's mean curvature at a shading point, from the normals its corners hold, as one
     * over a radius. It is above zero where the surface is convex on the side the ray met it, and
     * below where it is concave. A triangle without normals at its corners is flat, and so is any
     * other primitive.
     **/
    MOYA_KERNEL float curvature(const ShadingPoint & point) const {
        if (!point.found() || geometries[point.geometry].form != geometry::primitive::Form::Triangles) {
            return 0.0F;
        }
        const Primvar * found = primvar(point.geometry, kNormal);
        if (found == nullptr || found->components != 3U || found->perPrimitive != 3U) {
            return 0.0F;
        }
        glm::vec3 corner[3];
        corners(point.geometry, point.primitive, point.time, corner);
        const glm::mat3 turn { point.toWorld };
        // a normal moves by the inverse transpose of the transformation to the world
        const glm::mat3 normalTurn = glm::transpose(glm::inverse(turn));
        glm::vec3 normal[3];
        glm::vec3 sum { 0.0F };
        for (std::uint32_t at = 0; at < 3U; ++at) {
            const std::uint32_t from = found->first + (((point.primitive * 3U) + at) * 3U);
            const glm::vec3 turned = normalTurn * glm::vec3 { values[from], values[from + 1U], values[from + 2U] };
            normal[at] = glm::dot(turned, turned) > 0.0F ? glm::normalize(turned) : turned;
            sum += normal[at];
        }
        const glm::vec3 edge1 = turn * (corner[1] - corner[0]);
        const glm::vec3 edge2 = turn * (corner[2] - corner[0]);
        const float e11 = glm::dot(edge1, edge1);
        const float e12 = glm::dot(edge1, edge2);
        const float e22 = glm::dot(edge2, edge2);
        const float determinant = (e11 * e22) - (e12 * e12);
        if (!(std::fabs(determinant) > kLeastTangent)) {
            return 0.0F;
        }
        // how the normal turns along two directions in the surface, each as a sum of the two edges
        const glm::vec3 directions[2] = { point.frame.tangent, point.frame.bitangent };
        float bending = 0.0F;
        for (const glm::vec3 & direction : directions) {
            const float onFirst = glm::dot(direction, edge1);
            const float onSecond = glm::dot(direction, edge2);
            const float first = ((onFirst * e22) - (onSecond * e12)) / determinant;
            const float second = ((onSecond * e11) - (onFirst * e12)) / determinant;
            bending += glm::dot(((normal[1] - normal[0]) * first) + ((normal[2] - normal[0]) * second), direction);
        }
        // the corners' normals face the side the geometric normal does, or the other
        const bool facing = glm::dot(sum, point.geometric) >= 0.0F;
        const float mean = bending / 2.0F;
        return facing == point.front ? mean : -mean;
    }

    /**
     * The transformation into the world, at a time, of an instance in the world by its index in
     * the scene. False for an instance the surfaces do not hold.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the instance, then the time
    MOYA_KERNEL bool placement(std::uint32_t id, float time, glm::mat4 & toWorld) const {
        if (id >= byId.size || byId[id] == geometry::kNoPrimitive) {
            return false;
        }
        const Instance & instance = instances[byId[id]];
        toWorld = instance.keyCount > 1U ? geometry::transformAt(&keys[instance.keys], instance.keyCount, time) : instance.transform;
        return true;
    }

    /**
     * The transformation from the geometry a chain of instances places into the world at a time,
     * the geometry, and the chain's material. The chain names its instances by their indices in
     * the scene, outermost first. False for a chain the surfaces do not hold.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the chain, then the time
    MOYA_KERNEL bool chainAt(const std::uint32_t * chain, std::uint32_t depth, float time, glm::mat4 & toWorld, std::uint32_t & placed,
        std::uint32_t & material) const {  // NOLINT(bugprone-easily-swappable-parameters)
        if (depth == 0U || depth > geometry::kInstanceDepth) {
            return false;
        }
        toWorld = glm::mat4(1.0F);
        for (std::uint32_t level = 0; level < depth; ++level) {
            const std::uint32_t id = chain[level];
            if (id >= byId.size || byId[id] == geometry::kNoPrimitive) {
                return false;
            }
            const Instance & instance = instances[byId[id]];
            toWorld = toWorld * (instance.keyCount > 1U ? geometry::transformAt(&keys[instance.keys], instance.keyCount, time) : instance.transform);
            placed = instance.geometry;
            // the chain runs outermost first, so the innermost instance that names a material wins
            if (instance.material != kNoMaterial) {
                material = instance.material;
            }
        }
        return placed != kGroup && placed < geometries.size;
    }

    /** A triangle's corners at a time, in its geometry's own space. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the geometry, then the triangle
    /**
     * Where a hit's shading point lies in the world at another time, held to the shutter, as its
     * geometry and the instances that place it stand then. A triangle's point is its corners'
     * then, by its weights. A point or a curve's point moves in its geometry's own space as its
     * primitive's first point or control does, and a quadric's does not. A point that found
     * nothing stays where it is.
     **/
    MOYA_KERNEL glm::vec3 positionAt(const ShadingPoint & point, const geometry::Hit & hit, float time) const {
        const float then = held(time);
        glm::mat4 toWorld { 1.0F };
        std::uint32_t placed = kGroup;
        std::uint32_t material = kNoMaterial;
        if (!point.found() || !placing(hit, then, toWorld, placed, material)) {
            return point.position;
        }
        const Geometry & shape = geometries[placed];
        glm::vec3 local = glm::vec3(glm::inverse(point.toWorld) * glm::vec4(point.position, 1.0F));
        switch (shape.form) {
            case geometry::primitive::Form::Triangles: {
                glm::vec3 corner[3];
                corners(placed, hit.primitive, then, corner);
                local = (point.weights.x * corner[0]) + (point.weights.y * corner[1]) + (point.weights.z * corner[2]);
                break;
            }
            case geometry::primitive::Form::RoundPoints:
            case geometry::primitive::Form::FlatPoints:
                local += geometry::sampleAt(shape.points, points.data, times.data, hit.primitive, then) -
                    geometry::sampleAt(shape.points, points.data, times.data, hit.primitive, point.time);
                break;
            case geometry::primitive::Form::RoundLinearCurves:
            case geometry::primitive::Form::FlatLinearCurves:
            case geometry::primitive::Form::RoundCubicCurves:
            case geometry::primitive::Form::FlatCubicCurves: {
                const std::uint32_t first = hit.primitive * geometry::primitive::controlsOf(shape.form);
                local += glm::vec3(geometry::sampleAt(shape.controls, controls.data, times.data, first, then) -
                    geometry::sampleAt(shape.controls, controls.data, times.data, first, point.time));
                break;
            }
            case geometry::primitive::Form::Quadric:
                break;
        }
        return glm::vec3(toWorld * glm::vec4(local, 1.0F));
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the geometry, then its primitive
    MOYA_KERNEL void corners(std::uint32_t placed, std::uint32_t primitive, float time, glm::vec3 (&out)[3]) const {
        const Geometry & shape = geometries[placed];
        for (std::uint32_t corner = 0; corner < 3U; ++corner) {
            out[corner] = geometry::sampleAt(shape.points, points.data, times.data, (primitive * 3U) + corner, time);
        }
    }

 private:
    /**
     * A vector in a surface as its changes in `s` and `t`: the sum of the directions the point
     * moves in as `s` and `t` increase that comes nearest it, solved from their dot products.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the vector, then the directions
    MOYA_KERNEL static glm::vec2 inTexture(const glm::vec3 & vector, const glm::vec3 & alongS, const glm::vec3 & alongT, float ss, float st, float tt,
        float determinant) {
        const float onS = glm::dot(vector, alongS);
        const float onT = glm::dot(vector, alongT);
        return { ((onS * tt) - (onT * st)) / determinant, ((onT * ss) - (onS * st)) / determinant };
    }

    /**
     * How a triangle's point moves in the world as `s` and as `t` increase, from the `st` its
     * corners hold. False for a point not on such a triangle, or where `st` does not vary.
     **/
    MOYA_KERNEL bool textureDerivatives(const ShadingPoint & point, glm::vec3 & alongS, glm::vec3 & alongT) const {
        const Primvar * found = primvar(point.geometry, kTexture);
        if (geometries[point.geometry].form != geometry::primitive::Form::Triangles || found == nullptr || found->components != 2U ||
            found->perPrimitive != 3U) {
            return false;
        }
        glm::vec3 corner[3];
        corners(point.geometry, point.primitive, point.time, corner);
        const std::uint32_t first = found->first + (point.primitive * 6U);
        const float ds1 = values[first + 2U] - values[first];
        const float dt1 = values[first + 3U] - values[first + 1U];
        const float ds2 = values[first + 4U] - values[first];
        const float dt2 = values[first + 5U] - values[first + 1U];
        const float determinant = (ds1 * dt2) - (ds2 * dt1);
        if (!(std::fabs(determinant) > kLeastTangent)) {
            return false;
        }
        const glm::vec3 edge1 = corner[1] - corner[0];
        const glm::vec3 edge2 = corner[2] - corner[0];
        const glm::mat3 turn { point.toWorld };
        alongS = turn * (((edge1 * dt2) - (edge2 * dt1)) / determinant);
        alongT = turn * (((edge2 * ds1) - (edge1 * ds2)) / determinant);
        return true;
    }

    /**
     * The transformation from the geometry a hit's chain places into the world at a time, the
     * geometry, and the chain's material. False for a chain the surfaces do not hold.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the geometry, then its material
    MOYA_KERNEL bool placing(const geometry::Hit & hit, float time, glm::mat4 & toWorld, std::uint32_t & placed, std::uint32_t & material) const {
        return hit.found() && chainAt(hit.instances, hit.depth, time, toWorld, placed, material);
    }

    /**
     * A triangle's point, its geometric normal by its winding, its shading normal from an `N`
     * primvar, and the point a ray leaves it from, all in its own space.
     *
     * The leaving point is Hanika's: the point is moved out of each corner's tangent plane it lies
     * below, by that corner's weight. A triangle with no normal at each corner leaves from the
     * point itself.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the triangle, then what it gives
    MOYA_KERNEL void triangle(const Geometry & shape, std::uint32_t placed, const geometry::Hit & hit, float time, ShadingPoint & out, glm::vec3 & point,
        glm::vec3 & leave, glm::vec3 & geometric, glm::vec3 & shading) const {  // NOLINT(bugprone-easily-swappable-parameters)
        glm::vec3 corners[3];
        for (std::uint32_t corner = 0; corner < 3U; ++corner) {
            corners[corner] = geometry::sampleAt(shape.points, points.data, times.data, (hit.primitive * 3U) + corner, time);
        }
        out.weights = glm::vec3 { 1.0F - hit.uv.x - hit.uv.y, hit.uv.x, hit.uv.y };
        point = (corners[0] * out.weights.x) + (corners[1] * out.weights.y) + (corners[2] * out.weights.z);
        leave = point;
        geometric = glm::cross(corners[1] - corners[0], corners[2] - corners[0]);
        const Primvar * normal = primvar(placed, kNormal);
        if (normal == nullptr || normal->components != 3U) {
            return;
        }
        if (normal->perPrimitive != 3U) {
            float value[3] {};
            if (read(*normal, hit.primitive, out.weights, value, 3U)) {
                shading = glm::vec3 { value[0], value[1], value[2] };
            }
            return;
        }
        glm::vec3 offset { 0.0F };
        for (std::uint32_t corner = 0; corner < 3U; ++corner) {
            const std::uint32_t at = normal->first + (((hit.primitive * 3U) + corner) * 3U);
            glm::vec3 cornerNormal { values[at], values[at + 1U], values[at + 2U] };
            const float weight = out.weights[static_cast<glm::length_t>(corner)];
            shading += cornerNormal * weight;
            if (glm::dot(cornerNormal, cornerNormal) == 0.0F) {
                continue;
            }
            cornerNormal = glm::normalize(cornerNormal);
            if (glm::dot(cornerNormal, geometric) < 0.0F) {
                cornerNormal = -cornerNormal;
            }
            const float below = glm::dot(point - corners[corner], cornerNormal);
            offset -= cornerNormal * (weight * (below < 0.0F ? below : 0.0F));
        }
        leave = point + offset;
    }
};

};  // namespace v3d::moya::kernel::surface
