/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Parameters.h>

#include <array>
#include <cstdint>

namespace v3d::moya::material::internal {

/** An input of open_pbr_surface, the register its value goes in, and whether it is a colour. **/
struct OpenPbrInput {
    const char * name { nullptr };
    std::uint32_t at { 0 };
    bool colour { false };
};

/** Every input of open_pbr_surface that a program gives, but geometry_thin_walled. **/
inline constexpr std::array kOpenPbrInputs {
    OpenPbrInput { .name = "base_weight", .at = kernel::program::kBaseWeight, .colour = false },
    OpenPbrInput { .name = "base_diffuse_roughness", .at = kernel::program::kBaseDiffuseRoughness, .colour = false },
    OpenPbrInput { .name = "base_metalness", .at = kernel::program::kBaseMetalness, .colour = false },
    OpenPbrInput { .name = "specular_weight", .at = kernel::program::kSpecularWeight, .colour = false },
    OpenPbrInput { .name = "specular_roughness", .at = kernel::program::kSpecularRoughness, .colour = false },
    OpenPbrInput { .name = "specular_roughness_anisotropy", .at = kernel::program::kSpecularAnisotropy, .colour = false },
    OpenPbrInput { .name = "specular_ior", .at = kernel::program::kSpecularIor, .colour = false },
    OpenPbrInput { .name = "transmission_weight", .at = kernel::program::kTransmissionWeight, .colour = false },
    OpenPbrInput { .name = "subsurface_weight", .at = kernel::program::kSubsurfaceWeight, .colour = false },
    OpenPbrInput { .name = "coat_weight", .at = kernel::program::kCoatWeight, .colour = false },
    OpenPbrInput { .name = "coat_roughness", .at = kernel::program::kCoatRoughness, .colour = false },
    OpenPbrInput { .name = "coat_roughness_anisotropy", .at = kernel::program::kCoatAnisotropy, .colour = false },
    OpenPbrInput { .name = "coat_ior", .at = kernel::program::kCoatIor, .colour = false },
    OpenPbrInput { .name = "coat_darkening", .at = kernel::program::kCoatDarkening, .colour = false },
    OpenPbrInput { .name = "fuzz_weight", .at = kernel::program::kFuzzWeight, .colour = false },
    OpenPbrInput { .name = "fuzz_roughness", .at = kernel::program::kFuzzRoughness, .colour = false },
    OpenPbrInput { .name = "thin_film_weight", .at = kernel::program::kThinFilmWeight, .colour = false },
    OpenPbrInput { .name = "thin_film_thickness", .at = kernel::program::kThinFilmThickness, .colour = false },
    OpenPbrInput { .name = "thin_film_ior", .at = kernel::program::kThinFilmIor, .colour = false },
    OpenPbrInput { .name = "emission_luminance", .at = kernel::program::kEmissionLuminance, .colour = false },
    OpenPbrInput { .name = "base_color", .at = kernel::program::kBaseColour, .colour = true },
    OpenPbrInput { .name = "specular_color", .at = kernel::program::kSpecularColour, .colour = true },
    OpenPbrInput { .name = "transmission_color", .at = kernel::program::kTransmissionColour, .colour = true },
    OpenPbrInput { .name = "subsurface_color", .at = kernel::program::kSubsurfaceColour, .colour = true },
    OpenPbrInput { .name = "coat_color", .at = kernel::program::kCoatColour, .colour = true },
    OpenPbrInput { .name = "fuzz_color", .at = kernel::program::kFuzzColour, .colour = true },
    OpenPbrInput { .name = "emission_color", .at = kernel::program::kEmissionColour, .colour = true },
};

};  // namespace v3d::moya::material::internal
