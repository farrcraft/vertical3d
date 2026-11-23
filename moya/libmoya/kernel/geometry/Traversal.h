/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <type_traits>

namespace v3d::moya::kernel::geometry {

/**
 * A filter that counts every hit, for a search of the geometry alone.
 *
 * A filter decides whether a hit a traversal finds counts. counts(hit) is given a hit whole, its
 * distance, primitive, position on the primitive, time and chain of instances, and may change the
 * filter, as a shadow ray's gathers what it passes through.
 **/
struct EveryHit {
    MOYA_KERNEL static bool counts(const Hit & /*hit*/) {
        return true;
    }
};

/**
 * What a stage that traces rays takes as its traversal. Each device instantiates the stages with
 * its own, so kernel code names no device, and a test runs the same stages over any traversal.
 *
 * nearest(ray, filter) is the nearest hit within the ray's interval that the filter counts, or a
 * hit that found nothing. any(ray, filter) is whether a hit the filter counts lies within the
 * interval, and stops at the first, as a shadow ray needs. Both are const, since a launch traces
 * from every pixel at once.
 *
 * A traversal asks the filter about every hit on a surface whose material is cut out, and counts
 * every other hit without asking. It may ask about those too. It asks about a primitive at most
 * once for each ray, so that a filter that gathers sees each surface once.
 **/
template <typename T>
concept Traversal = requires(const T & traversal, const Ray & ray, EveryHit & filter) {
    requires std::is_same_v<decltype(traversal.nearest(ray, filter)), Hit>;
    requires std::is_same_v<decltype(traversal.any(ray, filter)), bool>;
};

};  // namespace v3d::moya::kernel::geometry
