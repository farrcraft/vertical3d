/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Displacement.h>
#include <moya/libmoya/kernel/program/Parameters.h>

#include <array>
#include <cstdint>

namespace v3d::moya::material::internal {

/** An input of a terminal's node, the register its value goes in, and how many it takes. **/
struct TerminalInput {
    const char * name { nullptr };
    std::uint32_t at { 0 };
    std::uint32_t width { 1 };
};

/** Every input of open_pbr_surface, which a program gives, but geometry_thin_walled. **/
inline constexpr std::array kOpenPbrInputs {
    TerminalInput { .name = "base_weight", .at = kernel::program::kBaseWeight },
    TerminalInput { .name = "base_diffuse_roughness", .at = kernel::program::kBaseDiffuseRoughness },
    TerminalInput { .name = "base_metalness", .at = kernel::program::kBaseMetalness },
    TerminalInput { .name = "specular_weight", .at = kernel::program::kSpecularWeight },
    TerminalInput { .name = "specular_roughness", .at = kernel::program::kSpecularRoughness },
    TerminalInput { .name = "specular_roughness_anisotropy", .at = kernel::program::kSpecularAnisotropy },
    TerminalInput { .name = "specular_ior", .at = kernel::program::kSpecularIor },
    TerminalInput { .name = "transmission_weight", .at = kernel::program::kTransmissionWeight },
    TerminalInput { .name = "subsurface_weight", .at = kernel::program::kSubsurfaceWeight },
    TerminalInput { .name = "coat_weight", .at = kernel::program::kCoatWeight },
    TerminalInput { .name = "coat_roughness", .at = kernel::program::kCoatRoughness },
    TerminalInput { .name = "coat_roughness_anisotropy", .at = kernel::program::kCoatAnisotropy },
    TerminalInput { .name = "coat_ior", .at = kernel::program::kCoatIor },
    TerminalInput { .name = "coat_darkening", .at = kernel::program::kCoatDarkening },
    TerminalInput { .name = "fuzz_weight", .at = kernel::program::kFuzzWeight },
    TerminalInput { .name = "fuzz_roughness", .at = kernel::program::kFuzzRoughness },
    TerminalInput { .name = "thin_film_weight", .at = kernel::program::kThinFilmWeight },
    TerminalInput { .name = "thin_film_thickness", .at = kernel::program::kThinFilmThickness },
    TerminalInput { .name = "thin_film_ior", .at = kernel::program::kThinFilmIor },
    TerminalInput { .name = "emission_luminance", .at = kernel::program::kEmissionLuminance },
    TerminalInput { .name = "geometry_opacity", .at = kernel::program::kGeometryOpacity },
    TerminalInput { .name = "transmission_depth", .at = kernel::program::kTransmissionDepth },
    TerminalInput { .name = "transmission_scatter_anisotropy", .at = kernel::program::kTransmissionScatterAnisotropy },
    TerminalInput { .name = "transmission_dispersion_scale", .at = kernel::program::kTransmissionDispersionScale },
    TerminalInput { .name = "transmission_dispersion_abbe_number", .at = kernel::program::kTransmissionDispersionAbbe },
    TerminalInput { .name = "subsurface_radius", .at = kernel::program::kSubsurfaceRadius },
    TerminalInput { .name = "subsurface_scatter_anisotropy", .at = kernel::program::kSubsurfaceScatterAnisotropy },
    TerminalInput { .name = "base_color", .at = kernel::program::kBaseColour, .width = 3 },
    TerminalInput { .name = "specular_color", .at = kernel::program::kSpecularColour, .width = 3 },
    TerminalInput { .name = "transmission_color", .at = kernel::program::kTransmissionColour, .width = 3 },
    TerminalInput { .name = "subsurface_color", .at = kernel::program::kSubsurfaceColour, .width = 3 },
    TerminalInput { .name = "coat_color", .at = kernel::program::kCoatColour, .width = 3 },
    TerminalInput { .name = "fuzz_color", .at = kernel::program::kFuzzColour, .width = 3 },
    TerminalInput { .name = "emission_color", .at = kernel::program::kEmissionColour, .width = 3 },
    TerminalInput { .name = "geometry_normal", .at = kernel::program::kGeometryNormal, .width = 3 },
    TerminalInput { .name = "geometry_tangent", .at = kernel::program::kGeometryTangent, .width = 3 },
    TerminalInput { .name = "geometry_coat_normal", .at = kernel::program::kGeometryCoatNormal, .width = 3 },
    TerminalInput { .name = "geometry_coat_tangent", .at = kernel::program::kGeometryCoatTangent, .width = 3 },
    TerminalInput { .name = "transmission_scatter", .at = kernel::program::kTransmissionScatter, .width = 3 },
    TerminalInput { .name = "subsurface_radius_scale", .at = kernel::program::kSubsurfaceRadiusScale, .width = 3 },
};

/** The inputs of a displacement of a float, which moves a point along its normal. **/
inline constexpr std::array kDisplacementFloatInputs {
    TerminalInput { .name = "displacement", .at = kernel::program::kDisplacementVector + 2U },
    TerminalInput { .name = "scale", .at = kernel::program::kDisplacementScale },
};

/** The inputs of a displacement of a vector3, along the tangent, the bitangent and the normal. **/
inline constexpr std::array kDisplacementVectorInputs {
    TerminalInput { .name = "displacement", .at = kernel::program::kDisplacementVector, .width = 3 },
    TerminalInput { .name = "scale", .at = kernel::program::kDisplacementScale },
};

};  // namespace v3d::moya::material::internal
