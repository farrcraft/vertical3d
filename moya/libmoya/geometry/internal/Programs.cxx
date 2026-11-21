/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/internal/Programs.h>

#include <moya/libmoya/geometry/Cache.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/program/Op.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/texture/Texture.h>
#include <moya/libmoya/kernel/volume/openpbr/Interior.h>
#include <moya/libmoya/kernel/volume/openpbr/Subsurface.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/material/Library.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace v3d::moya::geometry::internal {

namespace {

/**
 * Gives a compiled material the medium it encloses: its volume's, compiled from its volume
 * terminal when it has one. A layered material of no volume terminal takes the medium its
 * transmission gives and the one its subsurface gives, from its surface. Gives back the volume's
 * program, when it compiles. A volume that does not compile is reported, and the material
 * encloses no medium.
 **/
std::optional<material::Compiled> enclose(const scene::Scene & scene, const scene::Handle & handle, material::Compiled & compiled, std::vector<std::string> & names) {
    const scene::Handle volume = scene.value(handle, "volume").handleArray().front();
    if (!volume.valid() || !scene.contains(volume)) {
        if (compiled.material.layered != 0U) {
            compiled.material.surfaceMedia = 1U;
            compiled.material.medium = kernel::volume::openpbr::interior(compiled.material.surface);
            compiled.material.walked = kernel::volume::openpbr::walked(compiled.material.surface);
        }
        return std::nullopt;
    }
    try {
        material::Compiled medium = material::compileVolume(material::Library::installed(), scene, handle, names);
        compiled.material.medium = medium.medium;
        compiled.reports.insert(compiled.reports.end(), medium.reports.begin(), medium.reports.end());
        medium.reports.clear();
        return medium;
    } catch (const std::runtime_error & refused) {
        compiled.reports.push_back("material " + scene.name(handle) + " encloses no medium, because " + refused.what());
        return std::nullopt;
    }
}

/**
 * Whether a volume's program reads nothing but its fields and constants, so that its medium is
 * bounded where its fields are. Every operation up to Primvar reads only its operands.
 **/
bool boundedBy(const std::vector<kernel::program::Instruction> & instructions) {
    return std::ranges::all_of(instructions, [](const kernel::program::Instruction & instruction) {
        return static_cast<std::uint32_t>(instruction.op) <= static_cast<std::uint32_t>(kernel::program::Op::Primvar) &&
            instruction.op != kernel::program::Op::Image;
    });
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
 * compile is diffuse, and its report says why. The volume's program, when it compiles, goes into
 * volumes by the material's index in the scene.
 **/
std::optional<material::Compiled> surfaceOf(const scene::Scene & scene, const scene::Handle & handle, std::vector<std::string> & names,
    std::map<std::uint32_t, material::Compiled> & volumes) {
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
        if (std::optional<material::Compiled> volume = enclose(scene, handle, out, names)) {
            volumes.insert_or_assign(handle.index, std::move(*volume));
        }
        return out;
    } catch (const std::runtime_error & refused) {
        material::Compiled diffuse;
        diffuse.reports.push_back("material " + scene.name(handle) + " is diffuse, because " + refused.what());
        return diffuse;
    }
}

/** The scene's shader search path, or empty while it has no options. **/
std::string shaderSearchPathOf(const scene::Scene & scene) {
    const std::vector<scene::Handle> options = scene.objects("options");
    return options.empty() ? std::string() : scene.value(options.front(), "shaderSearchPath").stringArray().front();
}

/**
 * The scene's index of every node of a material's network and of every connection between them:
 * its terminals, and each node that feeds one, directly or through others.
 **/
std::set<std::uint32_t> networkOf(const scene::Scene & scene, const scene::Handle & material, const std::multimap<std::uint32_t, scene::Handle> & feeding) {
    std::set<std::uint32_t> out;
    std::vector<scene::Handle> nodes;
    for (const char * terminal : { "surface", "displacement", "volume" }) {
        const scene::Handle node = scene.value(material, terminal).handleArray().front();
        if (node.valid() && scene.contains(node)) {
            nodes.push_back(node);
        }
    }
    while (!nodes.empty()) {
        const scene::Handle node = nodes.back();
        nodes.pop_back();
        if (!out.insert(node.index).second) {
            continue;
        }
        const auto [first, last] = feeding.equal_range(node.index);
        for (auto it = first; it != last; ++it) {
            out.insert(it->second.index);
            const scene::Handle from = scene.value(it->second, "from").handleArray().front();
            if (from.valid() && scene.contains(from)) {
                nodes.push_back(from);
            }
        }
    }
    return out;
}

/** Every connection of the scene, by the scene's index of the node it feeds. **/
std::multimap<std::uint32_t, scene::Handle> feedingOf(const scene::Scene & scene) {
    std::multimap<std::uint32_t, scene::Handle> out;
    for (const scene::Handle & connection : scene.objects("connection")) {
        const scene::Handle to = scene.value(connection, "to").handleArray().front();
        if (to.valid()) {
            out.emplace(to.index, connection);
        }
    }
    return out;
}

};  // namespace

Programs::Programs(const scene::Scene & scene, Cache * cache) : names_ { "N", "Cs", "tangent", "st" } {
    std::map<std::uint32_t, Cache::Program> kept;
    std::multimap<std::uint32_t, scene::Handle> feeding;
    if (cache != nullptr) {
        cache->compiled_.clear();
        const std::string searchPath = shaderSearchPathOf(scene);
        if (searchPath != cache->shaderSearchPath_) {
            cache->programs_.clear();
            cache->shaderSearchPath_ = searchPath;
        }
        feeding = feedingOf(scene);
    }
    for (const scene::Handle & handle : scene.objects("material")) {
        Cache::Program program { .handle = handle };
        bool keep = false;
        if (cache != nullptr) {
            program.network = networkOf(scene, handle, feeding);
            const auto held = cache->programs_.find(handle.index);
            const auto touched = [&](const std::set<std::uint32_t> & indices) {
                return std::ranges::any_of(indices, [&](std::uint32_t index) { return cache->forgotten_.contains(index); });
            };
            keep = held != cache->programs_.end() && held->second.handle == handle && held->second.before == names_ &&
                !cache->forgotten_.contains(handle.index) && !touched(held->second.network) && !touched(program.network);
            if (keep) {
                program.before = std::move(held->second.before);
                program.after = std::move(held->second.after);
                program.displacement = std::move(held->second.displacement);
                program.surface = std::move(held->second.surface);
                program.volume = std::move(held->second.volume);
                names_ = program.after;
            }
        }
        if (!keep) {
            program.before = names_;
            program.displacement = displacementOf(scene, handle, names_);
            std::map<std::uint32_t, material::Compiled> volume;
            program.surface = surfaceOf(scene, handle, names_, volume);
            if (!volume.empty()) {
                program.volume = std::move(volume.begin()->second);
            }
            program.after = names_;
            if (cache != nullptr) {
                cache->compiled_.insert(handle.index);
            }
        }
        if (program.displacement) {
            displacements_.emplace(handle.index, *program.displacement);
        }
        if (program.surface) {
            compiled_.emplace(handle.index, *program.surface);
        }
        if (program.volume) {
            volumes_.emplace(handle.index, *program.volume);
        }
        if (cache != nullptr) {
            kept.emplace(handle.index, std::move(program));
        }
    }
    if (cache != nullptr) {
        cache->programs_ = std::move(kept);
        cache->forgotten_.clear();
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
    kernel::surface::Material out = append(held, prototypes);
    const auto volume = volumes_.find(material);
    if (volume != volumes_.end() && !volume->second.values.empty()) {
        out.mediumProgram = append(volume->second, prototypes).program;
        out.mediumBounded = boundedBy(volume->second.instructions) ? 1U : 0U;
    }
    return out;
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
