/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/CpuLibrary.h>
#include <moya/libmoya/device/cpu/Isa.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/view/Tracing.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::tracing {

/*
 * What the cases that build scenes and trace rays through them share: random numbers, sets of
 * triangles, scenes of meshes and instances, and running a check on every traversal.
 */

/** A random number in [0, 1) from a seed it moves on, the same on every compiler. **/
inline float next(std::uint32_t & seed) {
    seed = (seed * 1664525U) + 1013904223U;
    return static_cast<float>(seed >> 8U) / static_cast<float>(1U << 24U);
}

/**
 * Small triangles scattered through a cube from the origin to a side. Each corner lies up to size
 * from its triangle's centre along each axis.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cube's side, then a triangle's size
inline std::vector<glm::vec3> scattered(std::size_t count, float side, float size, std::uint32_t & seed) {
    std::vector<glm::vec3> points;
    points.reserve(count * 3);
    for (std::size_t i = 0; i < count; ++i) {
        const glm::vec3 centre = glm::vec3 { next(seed), next(seed), next(seed) } * side;
        for (int corner = 0; corner < 3; ++corner) {
            points.push_back(centre + (glm::vec3 { next(seed), next(seed), next(seed) } * size));
        }
    }
    return points;
}

/**
 * Long thin triangles along the diagonal of a square of side 100, each moved along x from the one
 * before and rising to a height. Their boxes overlap far more than the triangles do.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - how many, then how high
inline std::vector<glm::vec3> slivers(std::size_t count, float height) {
    std::vector<glm::vec3> points;
    points.reserve(count * 3);
    for (std::size_t i = 0; i < count; ++i) {
        const float offset = static_cast<float>(i) * (90.0F / static_cast<float>(count));
        points.emplace_back(offset, 0.0F, 0.0F);
        points.emplace_back(100.0F, 100.0F - offset, 0.0F);
        points.emplace_back(100.0F, 100.0F - offset, height);
    }
    return points;
}

inline glm::mat4 moved(float x, float y, float z) {  // NOLINT(bugprone-easily-swappable-parameters)
    return glm::translate(glm::mat4(1.0F), glm::vec3 { x, y, z });
}

/** A turn about a random axis, a scale from a half to two, and a move of up to a reach. **/
inline glm::mat4 randomly(float reach, std::uint32_t & seed) {
    const glm::vec3 axis = glm::normalize(glm::vec3 { next(seed), next(seed), next(seed) } - 0.5F);
    const glm::mat4 turn = glm::rotate(glm::mat4(1.0F), next(seed) * 6.28F, axis);
    const glm::mat4 scale = glm::scale(glm::mat4(1.0F), glm::vec3 { 0.5F + (next(seed) * 1.5F) });
    return moved(next(seed) * reach, next(seed) * reach, next(seed) * reach) * turn * scale;
}

/** The scene's camera, whose shutter opens at 0 and closes at 1. **/
inline void shutter(scene::Scene & scene) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(camera, "shutter", scene::Value::floats({ 0.0F, 1.0F }));
}

/**
 * A mesh of triangles of three points each, a face each, at each of its times. With no times it
 * holds still at the first sample.
 **/
inline scene::Handle mesh(scene::Scene & scene, const std::string & name, const std::vector<float> & times,
    const std::vector<std::vector<glm::vec3>> & samples) {
    const scene::Handle out = scene.create("mesh", name);
    std::vector<std::int32_t> indices;
    indices.reserve(samples.front().size());
    for (std::size_t i = 0; i < samples.front().size(); ++i) {
        indices.push_back(static_cast<std::int32_t>(i));
    }
    std::vector<scene::Value> values;
    values.reserve(samples.size());
    for (const std::vector<glm::vec3> & points : samples) {
        values.push_back(scene::Value::points(points));
    }
    if (times.empty()) {
        scene.set(out, "P", values.front());
    } else {
        scene.set(out, "P", times, values);
    }
    scene.set(out, "vertices", scene::Value::integers(std::vector<std::int32_t>(samples.front().size() / 3, 3)));
    scene.set(out, "indices", scene::Value::integers(indices));
    return out;
}

/** A mesh of triangles of three points each, a face each, that holds still. **/
inline scene::Handle mesh(scene::Scene & scene, const std::string & name, const std::vector<glm::vec3> & points) {
    return mesh(scene, name, {}, { points });
}

/**
 * An instance of a prototype, in a group or the world, with its transformation at each of its
 * times. With no times it holds still at the first transformation.
 **/
inline scene::Handle instanced(scene::Scene & scene, const std::string & name, const scene::Handle & prototype, const scene::Handle & group,
    const std::vector<float> & times, const std::vector<glm::mat4> & transforms) {
    const scene::Handle instance = scene.create("instance", name);
    scene.set(instance, "prototype", scene::Value::handle(prototype));
    if (group.valid()) {
        scene.set(instance, "group", scene::Value::handle(group));
    }
    if (times.empty()) {
        scene.set(instance, "transform", scene::Value::matrix(transforms.front()));
    } else {
        std::vector<scene::Value> values;
        values.reserve(transforms.size());
        for (const glm::mat4 & transform : transforms) {
            values.push_back(scene::Value::matrix(transform));
        }
        scene.set(instance, "transform", times, values);
    }
    return instance;
}

/** An instance of a prototype, in a group or the world, that holds still. **/
inline scene::Handle instanced(scene::Scene & scene, const std::string & name, const scene::Handle & prototype, const scene::Handle & group,
    const glm::mat4 & transform) {
    return instanced(scene, name, prototype, group, {}, { transform });
}

/** Whether one of the reports of what was placed holds a phrase. **/
template <typename Placed>
bool reported(const Placed & placed, const std::string & phrase) {
    return std::ranges::any_of(placed.reports, [&](const std::string & report) {
        return report.find(phrase) != std::string::npos;
    });
}

/** The instruction sets this processor runs, oldest first. **/
inline std::vector<device::cpu::Isa> runnable() {
    std::vector<device::cpu::Isa> out;
    for (const device::cpu::Isa isa : { device::cpu::Isa::Sse42, device::cpu::Isa::Avx2, device::cpu::Isa::Avx512 }) {
        if (isa <= device::newestIsa()) {
            out.push_back(isa);
        }
    }
    return out;
}

/** The name of a traversal at an instruction set, for a failure's message. **/
inline std::string tracerName(view::Tracer tracer, device::cpu::Isa isa) {
    return tracer == view::Tracer::BruteForce ? std::string("brute force") : device::isaName(isa);
}

/** Runs a check on the brute-force traversal, and on the hierarchy at each instruction set. **/
template <typename F>
void eachTraversal(F && check) {
    check(view::Tracer::BruteForce, device::cpu::Isa::Sse42);
    for (const device::cpu::Isa isa : runnable()) {
        check(view::Tracer::Hierarchy, isa);
    }
}

/**
 * Whether two traversals give one answer: both miss, or both meet the same primitive through the
 * same chain at distances within a thousandth. A traversal that moves the scene into the world
 * and one that moves each ray into an instance round apart, most where a ray grazes a triangle.
 **/
inline bool agrees(const kernel::geometry::Hit & want, const kernel::geometry::Hit & got) {
    if (want.found() != got.found()) {
        return false;
    }
    if (!want.found()) {
        return true;
    }
    const bool chain = want.depth == got.depth && std::equal(want.instances, want.instances + want.depth, got.instances);
    return chain && want.primitive == got.primitive && std::fabs(want.distance - got.distance) <= 1.0e-3F * std::max(1.0F, want.distance);
}

};  // namespace v3d::moya::tracing
