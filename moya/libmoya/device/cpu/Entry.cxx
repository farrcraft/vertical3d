/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

// The source of each of the CPU's libraries, compiled once for each instruction set with
// MOYA_CPU_SSE42, MOYA_CPU_AVX2 or MOYA_CPU_AVX512 defined. It defines the traversal and exports
// the table of entry points, which instantiate the kernel functions for this instruction set.

#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/device/cpu/Isa.h>
#include <moya/libmoya/device/cpu/Module.h>
#include <moya/libmoya/device/cpu/Traversal.h>
#include <moya/libmoya/device/cpu/internal/Boxes.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/BruteForce.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Trace.h>
#include <moya/libmoya/kernel/geometry/Triangle.h>
#include <moya/libmoya/kernel/view/HitView.h>

#include <bit>
#include <cstdint>

#include <glm/vec3.hpp>

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

/** Whether a ray meets a placed triangle within [start, end), and where. **/
bool meets(const Traversal & traversal, const kernel::geometry::Shear & shear, std::uint32_t triangle, float start, float end,
    kernel::geometry::TriangleHit & hit) {
    const kernel::Array<glm::vec3> & points = traversal.placed.points;
    return kernel::geometry::intersectTriangle(shear, points[triangle * 3U], points[(triangle * 3U) + 1U], points[(triangle * 3U) + 2U],
        start, end, hit);
}

/** Tests a leaf's triangles, keeping in out the nearest hit before closest. **/
void nearestInLeaf(const Traversal & traversal, const kernel::geometry::Shear & shear, const kernel::geometry::Ray & ray,
    std::uint32_t first, std::uint32_t count, float & closest, kernel::geometry::Hit & out) {
    for (std::uint32_t index = 0; index < count; ++index) {
        const std::uint32_t triangle = traversal.references[first + index];
        kernel::geometry::TriangleHit hit;
        if (!meets(traversal, shear, triangle, ray.start, closest, hit)) {
            continue;
        }
        closest = hit.distance;
        const kernel::geometry::Placement & placement = traversal.placed.placements[traversal.runs[triangle]];
        out.distance = hit.distance;
        out.uv = hit.uv;
        out.primitive = triangle - placement.first;
        out.depth = placement.depth;
        for (std::uint32_t level = 0; level < kernel::geometry::kInstanceDepth; ++level) {
            out.instances[level] = placement.instances[level];
        }
    }
}

};  // namespace

kernel::geometry::Hit Traversal::nearest(const kernel::geometry::Ray & ray) const {
    kernel::geometry::Hit out;
    out.time = ray.time;
    const internal::Reach reach = internal::Reach::of(ray);
    const kernel::geometry::Shear shear = kernel::geometry::Shear::of(ray);
    float closest = ray.end;

    std::uint32_t stack[kStack];
    float stackEntry[kStack];
    std::uint32_t depth = 0;
    stack[depth] = 0;
    stackEntry[depth++] = ray.start;
    while (depth > 0) {
        --depth;
        if (stackEntry[depth] > closest) {
            continue;
        }
        const Node & node = nodes[stack[depth]];
        Entered children[kWidth];
        const std::uint32_t count = reached(node, reach, ray.start, closest, children);
        // leaves first, nearest first; then the inner nodes, farthest pushed first
        for (std::uint32_t i = 0; i < count; ++i) {
            const std::uint32_t slot = slotOf(children[i]);
            if ((node.child[slot] & kLeaf) != 0) {
                nearestInLeaf(*this, shear, ray, node.child[slot] & ~kLeaf, node.count[slot], closest, out);
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
    return out;
}

bool Traversal::any(const kernel::geometry::Ray & ray) const {
    const internal::Reach reach = internal::Reach::of(ray);
    const kernel::geometry::Shear shear = kernel::geometry::Shear::of(ray);
    std::uint32_t stack[kStack];
    std::uint32_t depth = 0;
    stack[depth++] = 0;
    while (depth > 0) {
        --depth;
        const Node & node = nodes[stack[depth]];
        Entered children[kWidth];
        const std::uint32_t count = reached(node, reach, ray.start, ray.end, children);
        for (std::uint32_t i = 0; i < count; ++i) {
            const std::uint32_t slot = slotOf(children[i]);
            const std::uint32_t child = node.child[slot];
            if ((child & kLeaf) == 0) {
                if (depth < kStack) {
                    stack[depth++] = child;
                }
                continue;
            }
            const std::uint32_t first = child & ~kLeaf;
            for (std::uint32_t index = 0; index < node.count[slot]; ++index) {
                kernel::geometry::TriangleHit hit;
                if (meets(*this, shear, references[first + index], ray.start, ray.end, hit)) {
                    return true;
                }
            }
        }
    }
    return false;
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
