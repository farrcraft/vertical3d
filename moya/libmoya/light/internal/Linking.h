/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/link/LinkStep.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace v3d::moya::light::internal {

/**
 * The sets of linked lights the scene's shading points lie in, as kernel::light::link::Links reads
 * them, and the instances that cast each light's shadows.
 *
 * Set 0 is lit by every light whose includeRoot is 1. A light whose includeRoot is 0 lights only
 * the chains of instances that reach an instance it names, at any depth. Each set holds the lights
 * of set 0 and the lights that name an instance of the chain. first and steps lead a chain from
 * set to set, lit says whether each light lights each set, and count is how many sets there are.
 * casters holds, from each shadow-linked light's shadowFirst, the instances that cast its shadows.
 **/
struct Linking {
    std::vector<std::uint32_t> first;
    std::vector<kernel::light::link::LinkStep> steps;
    std::vector<std::uint8_t> lit;
    std::uint32_t count { 1 };
    std::vector<std::uint32_t> casters;

    /** Whether the light at a place, among count lights, lights a set. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the set, then the place
    bool lights(std::uint32_t set, std::uint32_t place, std::uint32_t lights) const {
        return lit[(static_cast<std::size_t>(set) * lights) + place] != 0U;
    }
};

/**
 * The scene's light links, from the handle of each light, by its place among the lights. Each
 * light whose shadowIncludeRoot is 0 is marked as shadow-linked, with the instances its
 * shadowInstances names as its casters.
 **/
Linking linking(const scene::Scene & scene, const geometry::Prototypes & prototypes, const std::vector<scene::Handle> & handles,
    std::vector<kernel::light::Light> & lights);

};  // namespace v3d::moya::light::internal
