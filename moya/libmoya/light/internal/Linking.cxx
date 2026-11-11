/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/light/internal/Linking.h>

#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/light/internal/LinkSets.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace v3d::moya::light::internal {

namespace {

/** The indices in the scene of the instances a light's parameter names, sorted, each once. **/
std::vector<std::uint32_t> named(const scene::Scene & scene, const scene::Handle & light, const char * name) {
    std::vector<std::uint32_t> out;
    for (const scene::Handle & instance : scene.value(light, name).handleArray()) {
        out.push_back(instance.index);
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

/** Whether an integer parameter of a light is 1. **/
bool on(const scene::Scene & scene, const scene::Handle & light, const char * name) {
    return scene.value(light, name).integerArray().front() != 0;
}

/**
 * Each light whose shadowIncludeRoot is 0 marked as shadow-linked, with the instances its
 * shadowInstances names appended to the casters from its shadowFirst.
 **/
void shadows(const scene::Scene & scene, const std::vector<scene::Handle> & handles, std::vector<kernel::light::Light> & lights,
    std::vector<std::uint32_t> & casters) {
    for (std::size_t place = 0; place < lights.size(); ++place) {
        kernel::light::Light & light = lights[place];
        light.shadowLinked = !on(scene, handles[place], "shadowIncludeRoot");
        if (light.shadowLinked) {
            const std::vector<std::uint32_t> names = named(scene, handles[place], "shadowInstances");
            light.shadowFirst = static_cast<std::uint32_t>(casters.size());
            light.shadowCount = static_cast<std::uint32_t>(names.size());
            casters.insert(casters.end(), names.begin(), names.end());
        }
    }
}

};  // namespace

Linking linking(const scene::Scene & scene, const geometry::Prototypes & prototypes, const std::vector<scene::Handle> & handles,
    std::vector<kernel::light::Light> & lights) {
    Linking out;
    shadows(scene, handles, lights, out.casters);
    const auto count = static_cast<std::uint32_t>(lights.size());
    std::vector<bool> everywhere(count, true);
    // the linked lights that name each instance, by its index in the scene, in the order of places
    std::map<std::uint32_t, std::vector<std::uint32_t>> naming;
    for (std::uint32_t place = 0; place < count; ++place) {
        everywhere[place] = on(scene, handles[place], "includeRoot");
        if (!everywhere[place]) {
            for (const std::uint32_t id : named(scene, handles[place], "instances")) {
                naming[id].push_back(place);
            }
        }
    }
    const LinkSets found(prototypes, naming);
    found.steps(out.first, out.steps);
    const std::vector<std::vector<std::uint32_t>> & sets = found.sets();
    out.count = static_cast<std::uint32_t>(sets.size());
    out.lit.assign(static_cast<std::size_t>(out.count) * count, 0U);
    for (std::uint32_t linked = 0; linked < out.count; ++linked) {
        for (std::uint32_t place = 0; place < count; ++place) {
            const bool lit = everywhere[place] || std::binary_search(sets[linked].begin(), sets[linked].end(), place);
            out.lit[(static_cast<std::size_t>(linked) * count) + place] = lit ? 1U : 0U;
        }
    }
    return out;
}

};  // namespace v3d::moya::light::internal
