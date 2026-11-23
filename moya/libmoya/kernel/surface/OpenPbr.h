/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::surface {

/**
 * OpenPBR Surface's parameters of its base, specular layer, coat, fuzz, thin film and
 * emission, each by OpenPBR's name and with OpenPBR's default. Colours are in ACEScg. A weight,
 * a roughness, a metalness and the darkening run from zero to one, an anisotropy from zero up to
 * one, and an index from one up. A thin film's thickness is in micrometres, and the emission's
 * luminance in nits. Subsurface scatters as the diffuse base, its colour mixed in by its weight.
 * The opacity runs from zero, where the surface is cut away, to one.
 *
 * The medium inside a surface that is not thin-walled comes from its transmission. The depth is
 * the distance at which white light inside takes the transmission colour, the scatter is a
 * scattering coefficient over that depth, and the anisotropy is the scatter's. A depth of zero is
 * no medium. A
 * subsurface's radius, times its scale in each channel, is the mean free path of the medium below
 * the surface, and its anisotropy that medium's. The dispersion's scale and Abbe number are held,
 * and nothing reads them.
 *
 * The coat works about its own normal and tangent, and every other layer about the surface's.
 * Each is a direction in world space, and one of no length is the shading point's own.
 **/
struct OpenPbr {
    float baseWeight { 1.0F };
    glm::vec3 baseColour { 0.8F };  // NOLINT(readability-magic-numbers) - OpenPBR's default
    float baseDiffuseRoughness { 0.0F };
    float baseMetalness { 0.0F };

    float specularWeight { 1.0F };
    glm::vec3 specularColour { 1.0F };
    float specularRoughness { 0.3F };  // NOLINT(readability-magic-numbers) - OpenPBR's default
    float specularAnisotropy { 0.0F };
    float specularIor { 1.5F };  // NOLINT(readability-magic-numbers) - OpenPBR's default

    float transmissionWeight { 0.0F };
    glm::vec3 transmissionColour { 1.0F };

    float subsurfaceWeight { 0.0F };
    glm::vec3 subsurfaceColour { 0.8F };  // NOLINT(readability-magic-numbers) - OpenPBR's default

    float coatWeight { 0.0F };
    glm::vec3 coatColour { 1.0F };
    float coatRoughness { 0.0F };
    float coatAnisotropy { 0.0F };
    float coatIor { 1.6F };  // NOLINT(readability-magic-numbers) - OpenPBR's default
    float coatDarkening { 1.0F };

    float fuzzWeight { 0.0F };
    glm::vec3 fuzzColour { 1.0F };
    float fuzzRoughness { 0.5F };

    float thinFilmWeight { 0.0F };
    float thinFilmThickness { 0.5F };
    float thinFilmIor { 1.4F };  // NOLINT(readability-magic-numbers) - OpenPBR's default

    float emissionLuminance { 0.0F };
    glm::vec3 emissionColour { 1.0F };

    glm::vec3 geometryNormal { 0.0F };
    glm::vec3 geometryTangent { 0.0F };
    glm::vec3 geometryCoatNormal { 0.0F };
    glm::vec3 geometryCoatTangent { 0.0F };
    float geometryOpacity { 1.0F };

    float transmissionDepth { 0.0F };
    glm::vec3 transmissionScatter { 0.0F };
    float transmissionScatterAnisotropy { 0.0F };
    float transmissionDispersionScale { 0.0F };
    float transmissionDispersionAbbe { 20.0F };  // NOLINT(readability-magic-numbers) - OpenPBR's default
    float subsurfaceRadius { 1.0F };
    glm::vec3 subsurfaceRadiusScale { 1.0F, 0.5F, 0.25F };  // NOLINT(readability-magic-numbers) - OpenPBR's default
    float subsurfaceScatterAnisotropy { 0.0F };

    /** One when the surface is a thin wall, with the same medium on both sides. **/
    std::uint32_t thinWalled { 0 };
};

};  // namespace v3d::moya::kernel::surface
