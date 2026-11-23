/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/link/LinkStep.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

namespace v3d::moya::kernel::light::link {

/**
 * Which lights light a shading point, and which instances cast a light's shadows.
 *
 * A shading point lies in a set of linked lights, which its chain of instances decides. Set 0
 * holds the lights that light every instance. A chain that reaches an instance a light names moves
 * on into a set that also holds that light. steps holds those moves, an instance's from first[id]
 * up to first[id + 1], by its index in the scene. lit holds, for each set, whether each
 * light lights it, by the set times the count of lights plus the light's place. count is the
 * count of lights.
 *
 * casters holds the instances, by their indices in the scene, that cast the shadows of each light
 * whose shadow link names them, sorted from each light's shadowFirst.
 **/
struct Links {
    Array<std::uint32_t> first;
    Array<LinkStep> steps;
    Array<std::uint8_t> lit;
    std::uint32_t count { 0 };
    Array<std::uint32_t> casters;

    /** The set of linked lights of a shading point a hit met, from its chain of instances. **/
    MOYA_KERNEL std::uint32_t setOf(const geometry::Hit & hit) const {
        std::uint32_t set = 0;
        for (std::uint32_t level = 0; level < hit.depth; ++level) {
            const std::uint32_t id = hit.instances[level];
            if (id + 1U >= first.size) {
                continue;
            }
            for (std::uint32_t step = first[id]; step < first[id + 1U]; ++step) {
                if (steps[step].from == set) {
                    set = steps[step].to;
                    break;
                }
            }
        }
        return set;
    }

    /** Whether the light at a place lights a set of linked lights. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the set, then the place
    MOYA_KERNEL bool lights(std::uint32_t set, std::uint32_t place) const {
        const std::uint32_t at = (set * count) + place;
        return at < lit.size && lit[at] != 0U;
    }

    /** Whether a hit's chain of instances casts a light's shadows. **/
    MOYA_KERNEL bool casts(const Light & light, const geometry::Hit & hit) const {
        if (!light.shadowLinked) {
            return true;
        }
        for (std::uint32_t level = 0; level < hit.depth; ++level) {
            const std::uint32_t id = hit.instances[level];
            std::uint32_t low = light.shadowFirst;
            std::uint32_t high = light.shadowFirst + light.shadowCount;
            while (low < high) {
                const std::uint32_t middle = low + ((high - low) / 2U);
                if (casters[middle] < id) {
                    low = middle + 1U;
                } else {
                    high = middle;
                }
            }
            if (low < light.shadowFirst + light.shadowCount && casters[low] == id) {
                return true;
            }
        }
        return false;
    }
};

};  // namespace v3d::moya::kernel::light::link
