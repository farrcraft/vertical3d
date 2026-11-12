/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/internal/Programs.h>

#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/material/Library.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace v3d::moya::geometry::internal {

Programs::Programs(const scene::Scene & scene) : names_ { "N", "Cs", "tangent", "st" } {
    for (const scene::Handle & handle : scene.objects("material")) {
        const scene::Handle surface = scene.value(handle, "surface").handleArray().front();
        if (!surface.valid() || !scene.contains(surface) || scene.value(surface, "language").stringArray().front() != "materialx") {
            continue;
        }
        try {
            compiled_.emplace(handle.index, material::compile(material::Library::installed(), scene, handle, names_));
        } catch (const std::runtime_error & refused) {
            material::Compiled diffuse;
            diffuse.reports.push_back("material " + scene.name(handle) + " is diffuse, because " + refused.what());
            compiled_.emplace(handle.index, std::move(diffuse));
        }
    }
}

kernel::surface::Material Programs::place(std::uint32_t material, Prototypes & prototypes, std::vector<std::string> & reports) const {
    const material::Compiled & held = compiled_.at(material);
    reports.insert(reports.end(), held.reports.begin(), held.reports.end());
    // the program's instructions and values follow every other material's
    kernel::surface::Material out = held.material;
    const auto values = static_cast<std::uint32_t>(prototypes.programValues.size());
    out.program.instructions = static_cast<std::uint32_t>(prototypes.instructions.size());
    out.program.values = values;
    prototypes.instructions.insert(prototypes.instructions.end(), held.instructions.begin(), held.instructions.end());
    prototypes.programValues.insert(prototypes.programValues.end(), held.values.begin(), held.values.end());
    for (const std::uint32_t at : held.colours) {
        prototypes.programColours.push_back(values + at);
    }
    prototypes.programSpaces.insert(prototypes.programSpaces.end(), held.spaces.begin(), held.spaces.end());
    return out;
}

};  // namespace v3d::moya::geometry::internal
