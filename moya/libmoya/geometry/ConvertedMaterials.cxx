/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/ConvertedMaterials.h>

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/Transform.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/program/Medium.h>
#include <moya/libmoya/kernel/program/Parameters.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/material/Fold.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::moya::geometry {

ConvertedMaterials ConvertedMaterials::of(const Prototypes & prototypes, const colour::Config & config) {
    ConvertedMaterials out { .materials = prototypes.materials, .programValues = prototypes.programValues };
    std::map<std::string, colour::Transform> transforms;
    const auto transformOf = [&](const std::string & space) -> const colour::Transform & {
        auto found = transforms.find(space);
        if (found == transforms.end()) {
            found = transforms.emplace(space, config.input(space)).first;
        }
        return found->second;
    };
    for (std::size_t i = 0; i < out.materials.size(); ++i) {
        if (out.materials[i].coloured != 0U) {
            out.materials[i].colour = transformOf(prototypes.materialSpaces[i]).apply(out.materials[i].colour);
        }
    }
    std::vector<float> & values = out.programValues;
    for (std::size_t i = 0; i < prototypes.programColours.size(); ++i) {
        const std::uint32_t at = prototypes.programColours[i];
        const glm::vec3 converted = transformOf(prototypes.programSpaces[i]).apply(glm::vec3 { values[at], values[at + 1U], values[at + 2U] });
        values[at] = converted.x;
        values[at + 1U] = converted.y;
        values[at + 2U] = converted.z;
    }
    for (kernel::surface::Material & material : out.materials) {
        if (material.layered != 0U && material.program.size != 0U) {
            const std::uint32_t thinWalled = material.surface.thinWalled;
            const material::Folded folded = material::fold(std::span(prototypes.instructions).subspan(material.program.instructions, material.program.count),
                std::span<const float>(values).subspan(material.program.values, material.program.size));
            material.surface = kernel::program::parameters(folded.values.data());
            material.surface.thinWalled = thinWalled;
            if (material.surfaceMedia != 0U) {
                material::enclose(folded, material);
            }
        }
        if (material.mediumProgram.size != 0U) {
            const kernel::program::Program & medium = material.mediumProgram;
            const material::Folded folded = material::fold(std::span(prototypes.instructions).subspan(medium.instructions, medium.count),
                std::span<const float>(values).subspan(medium.values, medium.size));
            material.medium = kernel::program::mediumOf(folded.values.data());
        }
    }
    return out;
}

};  // namespace v3d::moya::geometry
