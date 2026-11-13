/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Parameters.h>

#include <array>
#include <cstdint>

namespace v3d::moya::material::internal {

/** An input of open_pbr_surface, the register its value goes in, and how many it takes. **/
struct OpenPbrInput {
    const char * name { nullptr };
    std::uint32_t at { 0 };
    std::uint32_t width { 1 };
};

/** Every input of open_pbr_surface that a program gives, but geometry_thin_walled. **/
inline constexpr std::array kOpenPbrInputs {
    OpenPbrInput { .name = "base_weight", .at = kernel::program::kBaseWeight },
    OpenPbrInput { .name = "base_diffuse_roughness", .at = kernel::program::kBaseDiffuseRoughness },
    OpenPbrInput { .name = "base_metalness", .at = kernel::program::kBaseMetalness },
    OpenPbrInput { .name = "specular_weight", .at = kernel::program::kSpecularWeight },
    OpenPbrInput { .name = "specular_roughness", .at = kernel::program::kSpecularRoughness },
    OpenPbrInput { .name = "specular_roughness_anisotropy", .at = kernel::program::kSpecularAnisotropy },
    OpenPbrInput { .name = "specular_ior", .at = kernel::program::kSpecularIor },
    OpenPbrInput { .name = "transmission_weight", .at = kernel::program::kTransmissionWeight },
    OpenPbrInput { .name = "subsurface_weight", .at = kernel::program::kSubsurfaceWeight },
    OpenPbrInput { .name = "coat_weight", .at = kernel::program::kCoatWeight },
    OpenPbrInput { .name = "coat_roughness", .at = kernel::program::kCoatRoughness },
    OpenPbrInput { .name = "coat_roughness_anisotropy", .at = kernel::program::kCoatAnisotropy },
    OpenPbrInput { .name = "coat_ior", .at = kernel::program::kCoatIor },
    OpenPbrInput { .name = "coat_darkening", .at = kernel::program::kCoatDarkening },
    OpenPbrInput { .name = "fuzz_weight", .at = kernel::program::kFuzzWeight },
    OpenPbrInput { .name = "fuzz_roughness", .at = kernel::program::kFuzzRoughness },
    OpenPbrInput { .name = "thin_film_weight", .at = kernel::program::kThinFilmWeight },
    OpenPbrInput { .name = "thin_film_thickness", .at = kernel::program::kThinFilmThickness },
    OpenPbrInput { .name = "thin_film_ior", .at = kernel::program::kThinFilmIor },
    OpenPbrInput { .name = "emission_luminance", .at = kernel::program::kEmissionLuminance },
    OpenPbrInput { .name = "base_color", .at = kernel::program::kBaseColour, .width = 3 },
    OpenPbrInput { .name = "specular_color", .at = kernel::program::kSpecularColour, .width = 3 },
    OpenPbrInput { .name = "transmission_color", .at = kernel::program::kTransmissionColour, .width = 3 },
    OpenPbrInput { .name = "subsurface_color", .at = kernel::program::kSubsurfaceColour, .width = 3 },
    OpenPbrInput { .name = "coat_color", .at = kernel::program::kCoatColour, .width = 3 },
    OpenPbrInput { .name = "fuzz_color", .at = kernel::program::kFuzzColour, .width = 3 },
    OpenPbrInput { .name = "emission_color", .at = kernel::program::kEmissionColour, .width = 3 },
    OpenPbrInput { .name = "geometry_normal", .at = kernel::program::kGeometryNormal, .width = 3 },
    OpenPbrInput { .name = "geometry_tangent", .at = kernel::program::kGeometryTangent, .width = 3 },
    OpenPbrInput { .name = "geometry_coat_normal", .at = kernel::program::kGeometryCoatNormal, .width = 3 },
    OpenPbrInput { .name = "geometry_coat_tangent", .at = kernel::program::kGeometryCoatTangent, .width = 3 },
};

};  // namespace v3d::moya::material::internal
