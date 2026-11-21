/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/light/internal/Emitters.h>

#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Group.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>
#include <moya/libmoya/kernel/geometry/primitive/Quadric.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/light/Shading.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <type_traits>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::light::internal {

namespace {

/** Appends a value's bytes. **/
template <typename T>
void put(std::vector<std::uint8_t> & out, const T & value) {
    static_assert(std::is_trivially_copyable_v<T>);
    const std::size_t at = out.size();
    out.resize(at + sizeof(T));
    std::memcpy(&out[at], &value, sizeof(T));
}

/** Appends the values of a run of samples, and their times where there is more than one. **/
template <typename T>
void putSampled(std::vector<std::uint8_t> & out, const kernel::geometry::Sampled & held, const std::vector<T> & values, const std::vector<float> & times) {
    put(out, held.count);
    put(out, held.samples);
    for (std::uint32_t i = 0; i < held.count * held.samples; ++i) {
        put(out, values[held.first + i]);
    }
    for (std::uint32_t sample = 0; held.samples > 1U && sample < held.samples; ++sample) {
        put(out, times[held.times + sample]);
    }
}

/** Appends a quadric's dimensions. **/
void putQuadric(std::vector<std::uint8_t> & out, const kernel::geometry::primitive::Quadric & quadric) {
    put(out, quadric.shape);
    for (const float value : { quadric.radius, quadric.minorRadius, quadric.zmin, quadric.zmax, quadric.height, quadric.phimin, quadric.phimax, quadric.thetamax }) {
        put(out, value);
    }
    put(out, quadric.point1);
    put(out, quadric.point2);
}

};  // namespace

void chains(const geometry::Prototypes & prototypes, const std::vector<std::uint32_t> & members, std::vector<std::uint32_t> & chain,
    const std::function<void(const std::vector<std::uint32_t> &, std::uint32_t)> & found) {
    if (chain.size() == kernel::geometry::kInstanceDepth) {
        return;
    }
    for (const std::uint32_t member : members) {
        const geometry::Instance & instance = prototypes.instances[member];
        chain.push_back(member);
        if (instance.kind == geometry::Prototype::Group) {
            chains(prototypes, prototypes.groups[instance.prototype].instances, chain, found);
        } else if (instance.kind == geometry::Prototype::Geometry) {
            found(chain, instance.prototype);
        }
        chain.pop_back();
    }
}

std::uint32_t materialOf(const geometry::Prototypes & prototypes, const std::vector<std::uint32_t> & placed) {
    for (auto level = placed.rbegin(); level != placed.rend(); ++level) {
        const std::uint32_t material = prototypes.instances[*level].material;
        if (material != kernel::surface::kNoMaterial) {
            return material;
        }
    }
    return kernel::surface::kNoMaterial;
}

bool materialEmitter(const std::vector<kernel::surface::Material> & materials, std::uint32_t material, bool doubleSided, kernel::light::Light & out) {
    if (material == kernel::surface::kNoMaterial) {
        return false;
    }
    const kernel::surface::Material & held = materials[material];
    if (held.layered == 0U || !(held.surface.emissionLuminance > 0.0F)) {
        return false;
    }
    out = kernel::light::Light {};
    out.colour = held.surface.emissionColour;
    out.scale = held.surface.emissionLuminance;
    out.camera = true;
    out.object = true;
    out.twoSided = doubleSided;
    return true;
}

std::map<std::uint32_t, scene::Handle> meshEmitters(const scene::Scene & scene) {
    std::vector<scene::Handle> meshLights = scene.objects("meshLight");
    std::sort(meshLights.begin(), meshLights.end(), [](const scene::Handle & left, const scene::Handle & right) { return left.index < right.index; });
    std::map<std::uint32_t, scene::Handle> out;
    for (const scene::Handle & handle : meshLights) {
        const std::vector<scene::Handle> & named = scene.value(handle, "geometry").handleArray();
        if (!named.empty() && scene.contains(named.front())) {
            out.emplace(named.front().index, handle);
        }
    }
    return out;
}

std::vector<std::uint8_t> emitting(const geometry::Prototypes & prototypes, const std::vector<kernel::surface::Material> & materials,
    const std::map<std::uint32_t, scene::Handle> & named, const Shading & shading) {
    std::vector<std::uint8_t> out;
    put(out, prototypes.open);
    put(out, prototypes.close);
    std::vector<std::uint32_t> chain;
    chains(prototypes, prototypes.world.instances, chain, [&](const std::vector<std::uint32_t> & placed, std::uint32_t at) {
        const geometry::Geometry & shape = prototypes.geometries[at];
        const auto naming = named.find(shape.id);
        const std::uint32_t material = materialOf(prototypes, placed);
        kernel::light::Light glowing;
        const bool glows = materialEmitter(materials, material, prototypes.instances[placed.back()].doubleSided, glowing);
        const bool encloses =
            material != kernel::surface::kNoMaterial && material < shading.surfaces.materials.size && shading.surfaces.materials[material].encloses();
        if (naming == named.end() && !glows && !encloses) {
            return;
        }
        put(out, at);
        put(out, naming == named.end() ? scene::Handle {} : naming->second);
        put(out, glows);
        if (glows) {
            put(out, glowing.colour);
            put(out, glowing.scale);
            put(out, glowing.twoSided);
        }
        put(out, encloses);
        if (encloses) {
            put(out, shading.surfaces.materials[material].medium.emission);
        }
        put(out, static_cast<std::uint32_t>(placed.size()));
        for (const std::uint32_t level : placed) {
            const geometry::Instance & instance = prototypes.instances[level];
            put(out, instance.id);
            put(out, instance.transform);
            put(out, static_cast<std::uint32_t>(instance.keys.size()));
            for (const kernel::geometry::Key & key : instance.keys) {
                put(out, key);
            }
        }
        put(out, shape.form);
        put(out, shape.count);
        putSampled(out, shape.points, prototypes.points, prototypes.times);
        put(out, shape.quadric.count);
        put(out, shape.quadric.samples);
        for (std::uint32_t i = 0; i < shape.quadric.count * shape.quadric.samples; ++i) {
            putQuadric(out, prototypes.quadrics[shape.quadric.first + i]);
        }
        for (std::uint32_t sample = 0; shape.quadric.samples > 1U && sample < shape.quadric.samples; ++sample) {
            put(out, prototypes.times[shape.quadric.times + sample]);
        }
    });
    return out;
}

};  // namespace v3d::moya::light::internal
