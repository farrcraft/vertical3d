/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/cpu/Traversal.h>
#include <moya/libmoya/kernel/geometry/Hit.h>

#include <cstdint>

namespace v3d::moya::device::cpu::internal {

/**
 * What a search keeps as it enters instances and leaves them: the chain of instances the ray is
 * inside, and the filter it asks about hits on surfaces that are cut out. A search for the nearest
 * hit also keeps the nearest the filter counted so far.
 **/
template <typename F>
struct Search {
    /** The instances a ray is inside, outermost first, and whether their surfaces are cut out. **/
    struct Chain {
        std::uint32_t instances[kernel::geometry::kInstanceDepth] {};
        std::uint32_t depth { 0 };
        bool cutOut { false };
    };

    const Traversal & traversal;
    F & filter;
    Chain chain;

    /** The time of the ray the search began with, which every hit it offers holds. **/
    float time { 0.0F };

    float closest { 0.0F };
    kernel::geometry::Hit hit;

    /** A hit on a primitive the chain places, at a distance and a position on the primitive. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the primitive, then the distance
    kernel::geometry::Hit hitOn(std::uint32_t primitive, float distance, const glm::vec2 & uv) const {
        kernel::geometry::Hit out;
        out.distance = distance;
        out.primitive = primitive;
        out.uv = uv;
        out.time = time;
        out.depth = chain.depth;
        for (std::uint32_t level = 0; level < kernel::geometry::kInstanceDepth; ++level) {
            out.instances[level] = chain.instances[level];
        }
        return out;
    }

    /** Whether a hit counts. One on a solid surface does, and the filter decides the rest. **/
    bool counts(const kernel::geometry::Hit & candidate) {
        return !chain.cutOut || filter.counts(candidate);
    }
};

};  // namespace v3d::moya::device::cpu::internal
