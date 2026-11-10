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
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/kernel/surface/Geometry.h>
#include <moya/libmoya/kernel/surface/Instance.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/Primvar.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>

#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

/** The scene's surfaces as shading reads them, apart from any device's traversal. **/
namespace v3d::moya::kernel::surface {

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

    /** Every primvar's values. **/
    Array<float> values;

    /** The times the shutter opens and closes. **/
    float open { 0.0F };
    float close { 0.0F };

    /** A time held to the shutter, as a traversal holds a ray's. **/
    MOYA_KERNEL float held(float time) const {
        if (!(time > open)) {
            return open;
        }
        return time > close ? close : time;
    }

    /** A geometry's primvar of a name, or none. **/
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
            for (std::uint32_t c = 0; c < components; ++c) {
                out[c] = values[from + c];
            }
            return true;
        }
        if (source.perPrimitive != 3U) {
            return false;
        }
        for (std::uint32_t c = 0; c < components; ++c) {
            float sum = 0.0F;
            for (std::uint32_t corner = 0; corner < 3U; ++corner) {
                sum += weights[static_cast<glm::length_t>(corner)] * values[source.first + ((((primitive * 3U) + corner) * components) + c)];
            }
            out[c] = sum;
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
        return out;
    }

 private:
    /**
     * The transformation from the geometry a hit's chain places into the world at a time, the
     * geometry, and the chain's material. False for a chain the surfaces do not hold.
     **/
    MOYA_KERNEL bool placing(const geometry::Hit & hit, float time, glm::mat4 & toWorld, std::uint32_t & placed, std::uint32_t & material) const {
        if (!hit.found() || hit.depth == 0U || hit.depth > geometry::kInstanceDepth) {
            return false;
        }
        for (std::uint32_t level = 0; level < hit.depth; ++level) {
            const std::uint32_t id = hit.instances[level];
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
        glm::vec3 & leave, glm::vec3 & geometric, glm::vec3 & shading) const {
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
            glm::vec3 n { values[at], values[at + 1U], values[at + 2U] };
            const float weight = out.weights[static_cast<glm::length_t>(corner)];
            shading += n * weight;
            if (glm::dot(n, n) == 0.0F) {
                continue;
            }
            n = glm::normalize(n);
            if (glm::dot(n, geometric) < 0.0F) {
                n = -n;
            }
            const float below = glm::dot(point - corners[corner], n);
            offset -= n * (weight * (below < 0.0F ? below : 0.0F));
        }
        leave = point + offset;
    }
};

};  // namespace v3d::moya::kernel::surface
