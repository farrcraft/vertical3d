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
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/material/Library.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace v3d::moya::geometry::internal {

Programs::Programs(const scene::Scene & scene) : names_ { "N", "Cs", "tangent", "st" } {
    for (const scene::Handle & handle : scene.objects("material")) {
        const scene::Handle displacement = scene.value(handle, "displacement").handleArray().front();
        const bool displaces = displacement.valid() && scene.contains(displacement);
        const std::string displacedBy = displaces ? scene.value(displacement, "language").stringArray().front() : std::string();
        if (displacedBy == "materialx" || displacedBy == "ri") {
            try {
                displacements_.emplace(handle.index, material::compileDisplacement(material::Library::installed(), scene, handle, names_));
            } catch (const std::runtime_error & refused) {
                material::Compiled none;
                none.reports.push_back("material " + scene.name(handle) + " does not displace, because " + refused.what());
                displacements_.emplace(handle.index, std::move(none));
            }
        }
        const scene::Handle surface = scene.value(handle, "surface").handleArray().front();
        if (!surface.valid() || !scene.contains(surface)) {
            // a material of a volume and no surface is a boundary, which compiling marks
            const scene::Handle volume = scene.value(handle, "volume").handleArray().front();
            if (volume.valid() && scene.contains(volume)) {
                compiled_.emplace(handle.index, material::compile(material::Library::installed(), scene, handle, names_));
            }
            continue;
        }
        const std::string & language = scene.value(surface, "language").stringArray().front();
        if (language != "materialx" && language != "ri") {
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
