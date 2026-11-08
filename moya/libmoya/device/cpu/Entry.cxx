/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

// The source of each of the CPU's libraries, compiled once for each instruction set with
// MOYA_CPU_SSE42, MOYA_CPU_AVX2 or MOYA_CPU_AVX512 defined. It defines the traversal and exports
// the table of entry points, which instantiate the kernel functions for this instruction set.

#include <moya/libmoya/device/cpu/Instance.h>
#include <moya/libmoya/device/cpu/Isa.h>
#include <moya/libmoya/device/cpu/Module.h>
#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/device/cpu/Traversal.h>
#include <moya/libmoya/device/cpu/internal/Boxes.h>
#include <moya/libmoya/device/cpu/internal/Search.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Trace.h>
#include <moya/libmoya/kernel/geometry/Triangle.h>
#include <moya/libmoya/kernel/view/HitView.h>

#include <bit>
#include <cstdint>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#ifdef _WIN32
#define MOYA_CPU_EXPORT extern "C" __declspec(dllexport)
#else
#define MOYA_CPU_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#ifdef MOYA_CPU_AVX512
#define MOYA_CPU_ISA ::v3d::moya::device::cpu::Isa::Avx512
#elifdef MOYA_CPU_AVX2
#define MOYA_CPU_ISA ::v3d::moya::device::cpu::Isa::Avx2
#elifdef MOYA_CPU_SSE42
#define MOYA_CPU_ISA ::v3d::moya::device::cpu::Isa::Sse42
#else
#error "a CPU library names its instruction set"
#endif

namespace v3d::moya::device::cpu {

namespace {

/** The most nodes a traversal holds to visit. A hierarchy 64 deep pushes at most seven a level. **/
constexpr std::uint32_t kStack = 512;

/** A reached child: its slot and the distance the ray enters it at. **/
using Entered = std::uint64_t;

/** Packs a reached child so that ordering packs orders by distance, then slot. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the distance, then the slot
Entered pack(float entry, std::uint32_t slot) {
    // a distance that is not negative orders as its bits do
    const float clamped = entry > 0.0F ? entry : 0.0F;
    return (static_cast<std::uint64_t>(std::bit_cast<std::uint32_t>(clamped)) << 32U) | slot;
}

std::uint32_t slotOf(Entered entered) {
    return static_cast<std::uint32_t>(entered);
}

float entryOf(Entered entered) {
    return std::bit_cast<float>(static_cast<std::uint32_t>(entered >> 32U));
}

/** The children of a node the ray reaches, nearest first, and how many. **/
std::uint32_t reached(const Node & node, const internal::Reach & reach, float start, float end, Entered * out) {
    float entry[kWidth];
    std::uint32_t mask = internal::reach(node, reach, start, end, entry);
    std::uint32_t count = 0;
    while (mask != 0) {
        const auto slot = static_cast<std::uint32_t>(std::countr_zero(mask));
        mask &= mask - 1U;
        if (slot >= kWidth || node.child[slot] == kEmpty) {
            continue;
        }
        // insertion by distance, which is at most eight long
        const Entered packed = pack(entry[slot], slot);
        std::uint32_t at = count++;
        while (at > 0 && out[at - 1] > packed) {
            out[at] = out[at - 1];
            --at;
        }
        out[at] = packed;
    }
    return count;
}

/** Whether a ray enters an instance: one visible to the ray's kind, with room in the chain. **/
bool enters(const Instance & instance, const kernel::geometry::Ray & ray, std::uint32_t depth) {
    return (instance.visibility & ray.kind) != 0U && depth < kernel::geometry::kInstanceDepth;
}

/**
 * A ray moved into an instance's prototype's space. A distance along it reaches the point the same
 * distance along the ray it was moved from, so hits inside and outside an instance are compared by
 * their distances alone.
 **/
kernel::geometry::Ray into(const Instance & instance, const kernel::geometry::Ray & ray) {
    kernel::geometry::Ray out = ray;
    out.origin = glm::vec3(instance.toPrototype * glm::vec4(ray.origin, 1.0F));
    out.direction = glm::vec3(instance.toPrototype * glm::vec4(ray.direction, 0.0F));
    return out;
}

/** Whether a ray meets a triangle within [start, end), and where. **/
bool meets(const Traversal & traversal, const kernel::geometry::Shear & shear, std::uint32_t triangle, float start, float end,
    kernel::geometry::TriangleHit & hit) {
    const kernel::Array<glm::vec3> & points = traversal.points;
    return kernel::geometry::intersectTriangle(shear, points[triangle * 3U], points[(triangle * 3U) + 1U], points[(triangle * 3U) + 2U],
        start, end, hit);
}

void nearestFrom(internal::Nearest & search, std::uint32_t root, std::uint32_t first, const kernel::geometry::Ray & ray);

/** Tests the triangles of a mesh's leaf, keeping the nearest hit before the closest so far. **/
void nearestInMesh(internal::Nearest & search, const kernel::geometry::Shear & shear, const kernel::geometry::Ray & ray, std::uint32_t first,
    const Node & node, std::uint32_t slot) {
    const std::uint32_t leaf = node.child[slot] & ~kLeaf;
    for (std::uint32_t index = 0; index < node.count[slot]; ++index) {
        const std::uint32_t primitive = search.traversal.references[leaf + index];
        kernel::geometry::TriangleHit hit;
        if (!meets(search.traversal, shear, first + primitive, ray.start, search.closest, hit)) {
            continue;
        }
        search.closest = hit.distance;
        search.hit.distance = hit.distance;
        search.hit.uv = hit.uv;
        search.hit.primitive = primitive;
        search.hit.depth = search.chain.depth;
        for (std::uint32_t level = 0; level < kernel::geometry::kInstanceDepth; ++level) {
            search.hit.instances[level] = search.chain.instances[level];
        }
    }
}

/** Enters each instance of a group's leaf that the ray may enter, and searches its prototype. **/
void nearestInGroup(internal::Nearest & search, const kernel::geometry::Ray & ray, const Node & node, std::uint32_t slot) {
    const std::uint32_t leaf = node.child[slot] & ~kLeaf;
    for (std::uint32_t index = 0; index < node.count[slot]; ++index) {
        const Instance & instance = search.traversal.instances[search.traversal.references[leaf + index]];
        if (!enters(instance, ray, search.chain.depth)) {
            continue;
        }
        search.chain.instances[search.chain.depth++] = instance.id;
        nearestFrom(search, instance.root, instance.first, into(instance, ray));
        search.chain.instances[--search.chain.depth] = 0;
    }
}

/**
 * Searches one hierarchy from its root for the nearest hit before the closest so far. It is a
 * mesh's, whose leaves hold triangles from its first, or a group's for a first of kGroup, whose
 * leaves hold instances. The ray is in the hierarchy's own space.
 **/
void nearestFrom(internal::Nearest & search, std::uint32_t root, std::uint32_t first, const kernel::geometry::Ray & ray) {
    const internal::Reach reach = internal::Reach::of(ray);
    const kernel::geometry::Shear shear = kernel::geometry::Shear::of(ray);
    std::uint32_t stack[kStack];
    float stackEntry[kStack];
    std::uint32_t depth = 0;
    stack[depth] = root;
    stackEntry[depth++] = ray.start;
    while (depth > 0) {
        --depth;
        if (stackEntry[depth] > search.closest) {
            continue;
        }
        const Node & node = search.traversal.nodes[stack[depth]];
        Entered children[kWidth];
        const std::uint32_t count = reached(node, reach, ray.start, search.closest, children);
        // leaves first, nearest first; then the inner nodes, farthest pushed first
        for (std::uint32_t i = 0; i < count; ++i) {
            const std::uint32_t slot = slotOf(children[i]);
            if ((node.child[slot] & kLeaf) == 0) {
                continue;
            }
            if (first == kGroup) {
                nearestInGroup(search, ray, node, slot);
            } else {
                nearestInMesh(search, shear, ray, first, node, slot);
            }
        }
        for (std::uint32_t i = count; i > 0 && depth < kStack; --i) {
            const std::uint32_t slot = slotOf(children[i - 1]);
            if ((node.child[slot] & kLeaf) == 0) {
                stack[depth] = node.child[slot];
                stackEntry[depth++] = entryOf(children[i - 1]);
            }
        }
    }
}

bool anyFrom(const Traversal & traversal, std::uint32_t root, std::uint32_t first, const kernel::geometry::Ray & ray, std::uint32_t chain);

/**
 * Whether anything in a leaf lies on a ray within its interval: a triangle of a mesh's leaf from
 * its first, or anything inside an instance of a group's leaf that the ray may enter.
 **/
bool anyInLeaf(const Traversal & traversal, std::uint32_t first, const kernel::geometry::Shear & shear, const kernel::geometry::Ray & ray,
    std::uint32_t chain, const Node & node, std::uint32_t slot) {
    const std::uint32_t leaf = node.child[slot] & ~kLeaf;
    for (std::uint32_t index = 0; index < node.count[slot]; ++index) {
        const std::uint32_t reference = traversal.references[leaf + index];
        if (first != kGroup) {
            kernel::geometry::TriangleHit hit;
            if (meets(traversal, shear, first + reference, ray.start, ray.end, hit)) {
                return true;
            }
            continue;
        }
        const Instance & instance = traversal.instances[reference];
        if (enters(instance, ray, chain) && anyFrom(traversal, instance.root, instance.first, into(instance, ray), chain + 1)) {
            return true;
        }
    }
    return false;
}

/**
 * Whether anything lies on a ray within its interval in one hierarchy, read as nearestFrom()
 * reads it, inside a chain of instances of a depth.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - root, first triangle, chain depth
bool anyFrom(const Traversal & traversal, std::uint32_t root, std::uint32_t first, const kernel::geometry::Ray & ray, std::uint32_t chain) {
    const internal::Reach reach = internal::Reach::of(ray);
    const kernel::geometry::Shear shear = kernel::geometry::Shear::of(ray);
    std::uint32_t stack[kStack];
    std::uint32_t depth = 0;
    stack[depth++] = root;
    while (depth > 0) {
        --depth;
        const Node & node = traversal.nodes[stack[depth]];
        Entered children[kWidth];
        const std::uint32_t count = reached(node, reach, ray.start, ray.end, children);
        for (std::uint32_t i = 0; i < count; ++i) {
            const std::uint32_t slot = slotOf(children[i]);
            const std::uint32_t child = node.child[slot];
            if ((child & kLeaf) != 0) {
                if (anyInLeaf(traversal, first, shear, ray, chain, node, slot)) {
                    return true;
                }
            } else if (depth < kStack) {
                stack[depth++] = child;
            }
        }
    }
    return false;
}

};  // namespace

kernel::geometry::Hit Traversal::nearest(const kernel::geometry::Ray & ray) const {
    internal::Nearest search { .traversal = *this, .chain = {}, .closest = ray.end, .hit = {} };
    search.hit.time = ray.time;
    nearestFrom(search, world, kGroup, ray);
    return search.hit;
}

bool Traversal::any(const kernel::geometry::Ray & ray) const {
    return anyFrom(*this, world, kGroup, ray, 0);
}

};  // namespace v3d::moya::device::cpu

/** This library's table, which the CPU device reads when it loads it. **/
MOYA_CPU_EXPORT const v3d::moya::device::cpu::Module * moya_cpu_module() {
    static const v3d::moya::device::cpu::Module module { .version = v3d::moya::device::cpu::kModuleVersion,
        .isa = MOYA_CPU_ISA,
        .hits = &v3d::moya::kernel::view::hits<v3d::moya::device::cpu::Traversal>,
        .trace = &v3d::moya::kernel::geometry::traceRays<v3d::moya::device::cpu::Traversal> };
    return &module;
}
