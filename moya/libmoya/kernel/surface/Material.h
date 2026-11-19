/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Program.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/MaterialClosure.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/kernel/volume/Medium.h>

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
 *
 * A material encloses a medium when a path may cross its surface into it. A layered material
 * does when it is not thin-walled and its transmission's or its subsurface's weight may be more
 * than zero, and any other when it names a dielectric closure that is not thin. A boundary is a
 * material with a volume and no surface: it encloses its volume and has no BSDF, so a path passes
 * its surface by.
 * A traversal asks about a boundary's hits as it does a cut-out surface's.
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

    /** One when the transmission's weight may be more than zero, from its value or a node. **/
    std::uint32_t transmits { 0 };

    /** One when the subsurface's weight may be more than zero, from its value or a node. **/
    std::uint32_t subsurfaces { 0 };

    /** One when the material has a volume and no surface. **/
    std::uint32_t boundary { 0 };

    /**
     * The medium the material encloses: its volume's, when it has a volume, and otherwise the one
     * its OpenPBR transmission gives. A medium of no coefficients lets light through unchanged.
     **/
    volume::Medium medium;

    /**
     * The medium a layered material's subsurface gives, which a path walks once it enters the
     * surface through the subsurface's closure.
     **/
    volume::Medium walked;

    /**
     * One when the media the material encloses come from its OpenPBR surface: when it is layered
     * and has no volume terminal.
     **/
    std::uint32_t surfaceMedia { 0 };

    /**
     * The volume's program, whose values begin with a medium's, run where a volume of this
     * material reads its fields. mediumBounded is one when it reads nothing but its fields and
     * constants, so a bound on its fields bounds the medium it gives. A material of no volume has
     * a program of no values.
     **/
    program::Program mediumProgram;
    std::uint32_t mediumBounded { 1 };

    /** Whether a surface of this material may be cut out anywhere, or is a boundary. **/
    MOYA_KERNEL bool cutOut() const {
        return boundary != 0U || (layered != 0U && (programmedOpacity != 0U || surface.geometryOpacity < 1.0F));
    }

    /** Whether a path may cross a surface of this material into a medium it encloses. **/
    MOYA_KERNEL bool encloses() const {
        if (boundary != 0U) {
            return true;
        }
        if (layered != 0U) {
            return (transmits != 0U || subsurfaces != 0U) && surface.thinWalled == 0U;
        }
        for (std::uint32_t i = 0; i < count && i < kMostMaterialClosures; ++i) {
            if (closures[i].model == bsdf::Model::Dielectric && !closures[i].thin) {
                return true;
            }
        }
        return false;
    }
};

};  // namespace v3d::moya::kernel::surface
