/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Program.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/MaterialClosure.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>

#include <cstdint>
#include <limits>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::surface {

/** The material of an instance that names none. **/
inline constexpr std::uint32_t kNoMaterial = std::numeric_limits<std::uint32_t>::max();

/** The most closures a material names. **/
inline constexpr std::uint32_t kMostMaterialClosures = 8;

/**
 * A material as shading reads it: the colour its surface node gives, in ACEScg, when it gives
 * one, and the closures its BSDF holds. Each closure's weight is its scale times the surface's
 * colour. A material is one diffuse closure of scale one unless code sets others.
 *
 * A layered material is OpenPBR Surface of its parameters in place of its closures. Its program
 * gives the parameters at a shading point, and a program of no instructions gives the ones
 * surface holds.
 *
 * A layered material is cut out when its opacity may fall below one: when its program gives the
 * opacity, or the opacity it holds is less than one. A traversal asks about a hit on a surface
 * only when its material is cut out.
 **/
struct Material {
    glm::vec3 colour { 0.5F };

    /** One when the surface node gives a colour. **/
    std::uint32_t coloured { 0 };

    MaterialClosure closures[kMostMaterialClosures] {};
    std::uint32_t count { 1 };

    /** OpenPBR's parameters, and one when the material is layered by them. **/
    OpenPbr surface;
    std::uint32_t layered { 0 };
    program::Program program;

    /** One when the program gives the opacity, from a node that feeds it. **/
    std::uint32_t programmedOpacity { 0 };

    /** Whether a surface of this material may be cut out anywhere. **/
    MOYA_KERNEL bool cutOut() const {
        return layered != 0U && (programmedOpacity != 0U || surface.geometryOpacity < 1.0F);
    }
};

};  // namespace v3d::moya::kernel::surface
