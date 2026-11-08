/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/kernel/geometry/Ray.h>

#include <immintrin.h>

#include <cstddef>
#include <cstdint>

/**
 * What only the CPU's libraries compile, each for its own instruction set. Nothing outside them
 * includes these headers, so no instruction one library uses reaches another.
 **/
namespace v3d::moya::device::cpu::internal {

/**
 * A ray made ready for box tests, once for every node it reaches. Each axis names the planes a
 * ray running down it meets first and last, so a box is entered at its near planes and left at
 * its far ones whatever the direction's sign.
 **/
struct Reach {
    float origin[3] { 0.0F, 0.0F, 0.0F };
    float inverse[3] { 0.0F, 0.0F, 0.0F };
    bool backwards[3] { false, false, false };

    static Reach of(const kernel::geometry::Ray & ray) {
        Reach out;
        for (int axis = 0; axis < 3; ++axis) {
            out.origin[axis] = ray.origin[axis];
            out.inverse[axis] = 1.0F / ray.direction[axis];
            // the sign of a direction of zero, through its inverse's, so -0 runs backwards
            out.backwards[axis] = out.inverse[axis] < 0.0F;
        }
        return out;
    }
};

/**
 * Widens a box's far distance so that rounding in the three subtractions and products of a slab
 * test never misses a box the ray grazes, by Ize's bound of 1 + 2 gamma(3).
 **/
inline constexpr float kFarScale = 1.0F + (2.0F * ((3.0F * 5.96046448e-8F) / (1.0F - (3.0F * 5.96046448e-8F))));

/** The planes of one axis of a node, near first, for a ray running down it either way. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the near planes, then the far
inline void planes(const Node & node, int axis, bool backwards, const float *& near, const float *& far) {
    const float * lower = node.lowerZ;
    const float * upper = node.upperZ;
    switch (axis) {
        case 0:
            lower = node.lowerX;
            upper = node.upperX;
            break;
        case 1:
            lower = node.lowerY;
            upper = node.upperY;
            break;
        default:
            break;
    }
    near = backwards ? upper : lower;
    far = backwards ? lower : upper;
}

#ifdef MOYA_CPU_SSE42

/**
 * The children of a node whose boxes the ray enters within [start, end], one bit each, with the
 * distance it enters each at. An empty child's near plane lies beyond its far one, so no ray
 * reaches it. A product that is not a number, from a ray on a box's plane running along it, is
 * passed over by each maximum and minimum, which keep their second operand.
 **/
inline std::uint32_t reach(const Node & node, const Reach & ray, float start, float end, float * entry) {
    std::uint32_t mask = 0;
    for (int half = 0; half < 2; ++half) {
        const std::ptrdiff_t offset = static_cast<std::ptrdiff_t>(half) * 4;
        __m128 enter = _mm_set1_ps(start);
        __m128 leave = _mm_set1_ps(end);
        for (int axis = 0; axis < 3; ++axis) {
            const float * near = nullptr;
            const float * far = nullptr;
            planes(node, axis, ray.backwards[axis], near, far);
            const __m128 origin = _mm_set1_ps(ray.origin[axis]);
            const __m128 inverse = _mm_set1_ps(ray.inverse[axis]);
            const __m128 nearT = _mm_mul_ps(_mm_sub_ps(_mm_loadu_ps(near + offset), origin), inverse);
            const __m128 farT = _mm_mul_ps(_mm_mul_ps(_mm_sub_ps(_mm_loadu_ps(far + offset), origin), inverse), _mm_set1_ps(kFarScale));
            enter = _mm_max_ps(nearT, enter);
            leave = _mm_min_ps(farT, leave);
        }
        _mm_storeu_ps(entry + offset, enter);
        mask |= static_cast<std::uint32_t>(_mm_movemask_ps(_mm_cmple_ps(enter, leave))) << (half * 4);
    }
    return mask;
}

#else

/**
 * The children of a node whose boxes the ray enters within [start, end], one bit each, with the
 * distance it enters each at. An empty child's near plane lies beyond its far one, so no ray
 * reaches it. A product that is not a number, from a ray on a box's plane running along it, is
 * passed over by each maximum and minimum, which keep their second operand.
 **/
inline std::uint32_t reach(const Node & node, const Reach & ray, float start, float end, float * entry) {
    __m256 enter = _mm256_set1_ps(start);
    __m256 leave = _mm256_set1_ps(end);
    for (int axis = 0; axis < 3; ++axis) {
        const float * near = nullptr;
        const float * far = nullptr;
        planes(node, axis, ray.backwards[axis], near, far);
        const __m256 origin = _mm256_set1_ps(ray.origin[axis]);
        const __m256 inverse = _mm256_set1_ps(ray.inverse[axis]);
        const __m256 nearT = _mm256_mul_ps(_mm256_sub_ps(_mm256_loadu_ps(near), origin), inverse);
        const __m256 farT = _mm256_mul_ps(_mm256_mul_ps(_mm256_sub_ps(_mm256_loadu_ps(far), origin), inverse), _mm256_set1_ps(kFarScale));
        enter = _mm256_max_ps(nearT, enter);
        leave = _mm256_min_ps(farT, leave);
    }
    _mm256_storeu_ps(entry, enter);
    return static_cast<std::uint32_t>(_mm256_movemask_ps(_mm256_cmp_ps(enter, leave, _CMP_LE_OQ)));
}

#endif

};  // namespace v3d::moya::device::cpu::internal
