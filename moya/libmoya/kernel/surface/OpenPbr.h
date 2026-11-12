/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::surface {

/**
 * OpenPBR Surface's parameters of its base, specular layer, coat, fuzz and thin film, each by
 * OpenPBR's name and with OpenPBR's default. Colours are in ACEScg. A weight, a roughness, a
 * metalness and the darkening run from zero to one, an anisotropy from zero up to one, and an
 * index from one up. A thin film's thickness is in micrometres. Subsurface scatters as the
 * diffuse base, its colour mixed in by its weight.
 **/
struct OpenPbr {
    float baseWeight { 1.0F };
    glm::vec3 baseColour { 0.8F };
    float baseDiffuseRoughness { 0.0F };
    float baseMetalness { 0.0F };

    float specularWeight { 1.0F };
    glm::vec3 specularColour { 1.0F };
    float specularRoughness { 0.3F };
    float specularAnisotropy { 0.0F };
    float specularIor { 1.5F };

    float transmissionWeight { 0.0F };
    glm::vec3 transmissionColour { 1.0F };

    float subsurfaceWeight { 0.0F };
    glm::vec3 subsurfaceColour { 0.8F };

    float coatWeight { 0.0F };
    glm::vec3 coatColour { 1.0F };
    float coatRoughness { 0.0F };
    float coatAnisotropy { 0.0F };
    float coatIor { 1.6F };
    float coatDarkening { 1.0F };

    float fuzzWeight { 0.0F };
    glm::vec3 fuzzColour { 1.0F };
    float fuzzRoughness { 0.5F };

    float thinFilmWeight { 0.0F };
    float thinFilmThickness { 0.5F };
    float thinFilmIor { 1.4F };

    /** One when the surface is a thin wall, with the same medium on both sides. **/
    std::uint32_t thinWalled { 0 };
};

};  // namespace v3d::moya::kernel::surface
