/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/internal/Programs.h>

#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/program/Op.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/texture/Texture.h>
#include <moya/libmoya/kernel/volume/Interior.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/material/Library.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace v3d::moya::geometry::internal {

namespace {

/**
 * Gives a compiled material the medium it encloses: its volume's, compiled from its volume
 * terminal when it has one, and otherwise the one a layered material's OpenPBR transmission
 * gives. A volume that does not compile is reported, and the material encloses no medium.
 **/
void enclose(const scene::Scene & scene, const scene::Handle & handle, material::Compiled & compiled, std::vector<std::string> & names) {
    const scene::Handle volume = scene.value(handle, "volume").handleArray().front();
    if (!volume.valid() || !scene.contains(volume)) {
        if (compiled.material.layered != 0U) {
            compiled.material.medium = kernel::volume::interior(compiled.material.surface);
        }
        return;
    }
    try {
        const material::Compiled medium = material::compileVolume(material::Library::installed(), scene, handle, names);
        compiled.material.medium = medium.medium;
        compiled.reports.insert(compiled.reports.end(), medium.reports.begin(), medium.reports.end());
    } catch (const std::runtime_error & refused) {
        compiled.reports.push_back("material " + scene.name(handle) + " encloses no medium, because " + refused.what());
    }
}

/**
 * A material's displacement compiled, when its displacement node is a MaterialX or an RI node.
 * One that does not compile does not displace, and its report says why.
 **/
std::optional<material::Compiled> displacementOf(const scene::Scene & scene, const scene::Handle & handle, std::vector<std::string> & names) {
    const scene::Handle displacement = scene.value(handle, "displacement").handleArray().front();
    const bool displaces = displacement.valid() && scene.contains(displacement);
    const std::string displacedBy = displaces ? scene.value(displacement, "language").stringArray().front() : std::string();
    if (displacedBy != "materialx" && displacedBy != "ri") {
        return std::nullopt;
    }
    try {
        return material::compileDisplacement(material::Library::installed(), scene, handle, names);
    } catch (const std::runtime_error & refused) {
        material::Compiled none;
        none.reports.push_back("material " + scene.name(handle) + " does not displace, because " + refused.what());
        return none;
    }
}

/**
 * A material compiled with the medium it encloses, when its surface node is a MaterialX or an RI
 * node, or it has a volume and no surface, which makes it a boundary. A surface that does not
 * compile is diffuse, and its report says why.
 **/
std::optional<material::Compiled> surfaceOf(const scene::Scene & scene, const scene::Handle & handle, std::vector<std::string> & names) {
    const scene::Handle surface = scene.value(handle, "surface").handleArray().front();
    if (!surface.valid() || !scene.contains(surface)) {
        const scene::Handle volume = scene.value(handle, "volume").handleArray().front();
        if (!volume.valid() || !scene.contains(volume)) {
            return std::nullopt;
        }
    } else {
        const std::string & language = scene.value(surface, "language").stringArray().front();
        if (language != "materialx" && language != "ri") {
            return std::nullopt;
        }
    }
    try {
        material::Compiled out = material::compile(material::Library::installed(), scene, handle, names);
        enclose(scene, handle, out, names);
        return out;
    } catch (const std::runtime_error & refused) {
        material::Compiled diffuse;
        diffuse.reports.push_back("material " + scene.name(handle) + " is diffuse, because " + refused.what());
        return diffuse;
    }
}

};  // namespace

Programs::Programs(const scene::Scene & scene) : names_ { "N", "Cs", "tangent", "st" } {
    for (const scene::Handle & handle : scene.objects("material")) {
        if (std::optional<material::Compiled> displaced = displacementOf(scene, handle, names_)) {
            displacements_.emplace(handle.index, std::move(*displaced));
        }
        if (std::optional<material::Compiled> surfaced = surfaceOf(scene, handle, names_)) {
            compiled_.emplace(handle.index, std::move(*surfaced));
        }
    }
    // a channel can read a primvar that no program reads
    for (const scene::Handle & output : scene.objects("output")) {
        for (const scene::Handle & channel : scene.value(output, "channels").handleArray()) {
            const std::string & name = scene.value(channel, "sourceName").stringArray().front();
            if (scene.value(channel, "sourceType").stringArray().front() == "primvar" && !name.empty() && std::ranges::find(names_, name) == names_.end()) {
                names_.push_back(name);
            }
        }
    }
}

kernel::surface::Material Programs::place(std::uint32_t material, Prototypes & prototypes, std::vector<std::string> & reports) const {
    const material::Compiled & held = compiled_.at(material);
    reports.insert(reports.end(), held.reports.begin(), held.reports.end());
    return append(held, prototypes);
}

kernel::surface::Material Programs::append(const material::Compiled & held, Prototypes & prototypes) {
    // the program's instructions and values follow every other material's
    kernel::surface::Material out = held.material;
    const auto values = static_cast<std::uint32_t>(prototypes.programValues.size());
    out.program.instructions = static_cast<std::uint32_t>(prototypes.instructions.size());
    out.program.values = values;
    // an image's number follows every other material's images
    const auto images = static_cast<std::uint32_t>(prototypes.images.size());
    for (kernel::program::Instruction instruction : held.instructions) {
        if (instruction.op == kernel::program::Op::Image && instruction.in[2] != kernel::texture::kNoTexture) {
            instruction.in[2] += images;
        }
        prototypes.instructions.push_back(instruction);
    }
    prototypes.images.insert(prototypes.images.end(), held.images.begin(), held.images.end());
    prototypes.programValues.insert(prototypes.programValues.end(), held.values.begin(), held.values.end());
    for (const std::uint32_t at : held.colours) {
        prototypes.programColours.push_back(values + at);
    }
    prototypes.programSpaces.insert(prototypes.programSpaces.end(), held.spaces.begin(), held.spaces.end());
    return out;
}

};  // namespace v3d::moya::geometry::internal
