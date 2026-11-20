/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/surface/Material.h>

#include <vector>

namespace v3d::moya::colour {
class Config;
};  // namespace v3d::moya::colour

namespace v3d::moya::geometry {

struct Prototypes;

/**
 * The prototypes' materials and the values their programs start from, with every colour converted
 * into ACEScg from the space it names: an RI material's colour, and each colour constant of a
 * program. A layered material's surface holds the parameters its program gives from its converted
 * values, each that is the same at every shading point folded as material::fold() folds it, so
 * what is drawn and what is lit agree. A layered material of no volume terminal holds the media
 * material::enclose() gives that surface. A hair's material holds the parameters its program gives
 * from its converted values, folded the same way. A material with a volume program holds the
 * medium that program gives from its converted values, folded the same way, so a closed surface
 * encloses the medium a volume of the material reads.
 **/
struct ConvertedMaterials {
    std::vector<kernel::surface::Material> materials;
    std::vector<float> programValues;

    /**
     * @throws std::runtime_error, from OpenColorIO, when the configuration has no space a colour
     *         names
     **/
    static ConvertedMaterials of(const Prototypes & prototypes, const colour::Config & config);
};

};  // namespace v3d::moya::geometry
