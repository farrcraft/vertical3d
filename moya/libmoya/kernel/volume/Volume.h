/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/volume/Field.h>

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::volume {

/** The most fields one volume reads. **/
inline constexpr std::uint32_t kMostFields = 4;

/**
 * A volume as a chain of instances places it. The chain names its instances by their indices in
 * the scene, outermost first, and ends in the instance that places the volume. Its medium is its
 * material's volume program, run with its fields. A volume of no material, or whose material has
 * no volume program, scatters its density field, the same at every wavelength and every way.
 *
 * visibility holds the kinds of ray that see it, each a bit of geometry's ray kinds. walked is the
 * field whose cells a ray walks to bound the medium, its density where it has one.
 * A medium is bounded over a cell by its program run with the walked field at the cell's most and
 * every other field at its grid's most. A volume that is not bounded takes that bound with the
 * walked field at its grid's most too, over the whole volume. lower and upper bound every field,
 * in the volume's own space.
 **/
struct Volume {
    std::uint32_t chain[geometry::kInstanceDepth] {};
    std::uint32_t depth { 0 };
    std::uint32_t visibility { 0 };
    std::uint32_t material { surface::kNoMaterial };
    Field fields[kMostFields] {};
    std::uint32_t fieldCount { 0 };
    std::uint32_t walked { 0 };
    std::uint32_t bounded { 1 };
    glm::vec3 lower { 0.0F };
    glm::vec3 upper { 0.0F };
};

};  // namespace v3d::moya::kernel::volume
