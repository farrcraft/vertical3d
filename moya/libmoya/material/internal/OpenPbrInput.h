/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/surface/OpenPbr.h>

#include <array>

#include <glm/vec3.hpp>

namespace v3d::moya::material::internal {

/** An input of open_pbr_surface, and the float or colour of kernel::surface::OpenPbr it fills. **/
struct OpenPbrInput {
    const char * name { nullptr };
    float kernel::surface::OpenPbr::*number { nullptr };
    glm::vec3 kernel::surface::OpenPbr::*colour { nullptr };
};

/** Every input of open_pbr_surface that OpenPbr holds, but geometry_thin_walled. **/
inline constexpr std::array kOpenPbrInputs {
    OpenPbrInput { .name = "base_weight", .number = &kernel::surface::OpenPbr::baseWeight },
    OpenPbrInput { .name = "base_diffuse_roughness", .number = &kernel::surface::OpenPbr::baseDiffuseRoughness },
    OpenPbrInput { .name = "base_metalness", .number = &kernel::surface::OpenPbr::baseMetalness },
    OpenPbrInput { .name = "specular_weight", .number = &kernel::surface::OpenPbr::specularWeight },
    OpenPbrInput { .name = "specular_roughness", .number = &kernel::surface::OpenPbr::specularRoughness },
    OpenPbrInput { .name = "specular_roughness_anisotropy", .number = &kernel::surface::OpenPbr::specularAnisotropy },
    OpenPbrInput { .name = "specular_ior", .number = &kernel::surface::OpenPbr::specularIor },
    OpenPbrInput { .name = "transmission_weight", .number = &kernel::surface::OpenPbr::transmissionWeight },
    OpenPbrInput { .name = "subsurface_weight", .number = &kernel::surface::OpenPbr::subsurfaceWeight },
    OpenPbrInput { .name = "coat_weight", .number = &kernel::surface::OpenPbr::coatWeight },
    OpenPbrInput { .name = "coat_roughness", .number = &kernel::surface::OpenPbr::coatRoughness },
    OpenPbrInput { .name = "coat_roughness_anisotropy", .number = &kernel::surface::OpenPbr::coatAnisotropy },
    OpenPbrInput { .name = "coat_ior", .number = &kernel::surface::OpenPbr::coatIor },
    OpenPbrInput { .name = "coat_darkening", .number = &kernel::surface::OpenPbr::coatDarkening },
    OpenPbrInput { .name = "fuzz_weight", .number = &kernel::surface::OpenPbr::fuzzWeight },
    OpenPbrInput { .name = "fuzz_roughness", .number = &kernel::surface::OpenPbr::fuzzRoughness },
    OpenPbrInput { .name = "thin_film_weight", .number = &kernel::surface::OpenPbr::thinFilmWeight },
    OpenPbrInput { .name = "thin_film_thickness", .number = &kernel::surface::OpenPbr::thinFilmThickness },
    OpenPbrInput { .name = "thin_film_ior", .number = &kernel::surface::OpenPbr::thinFilmIor },
    OpenPbrInput { .name = "emission_luminance", .number = &kernel::surface::OpenPbr::emissionLuminance },
    OpenPbrInput { .name = "base_color", .colour = &kernel::surface::OpenPbr::baseColour },
    OpenPbrInput { .name = "specular_color", .colour = &kernel::surface::OpenPbr::specularColour },
    OpenPbrInput { .name = "transmission_color", .colour = &kernel::surface::OpenPbr::transmissionColour },
    OpenPbrInput { .name = "subsurface_color", .colour = &kernel::surface::OpenPbr::subsurfaceColour },
    OpenPbrInput { .name = "coat_color", .colour = &kernel::surface::OpenPbr::coatColour },
    OpenPbrInput { .name = "fuzz_color", .colour = &kernel::surface::OpenPbr::fuzzColour },
    OpenPbrInput { .name = "emission_color", .colour = &kernel::surface::OpenPbr::emissionColour },
};

};  // namespace v3d::moya::material::internal
