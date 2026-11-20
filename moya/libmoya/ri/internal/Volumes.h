/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/rib/Parameters.h>
#include <api/rib/requests/Volume.h>
#include <moya/libmoya/ri/internal/Warnings.h>

#include <optional>
#include <string>
#include <vector>

namespace v3d::moya::ri::internal {

/** The file a volume reads, and the grids in it, each read by its own name. **/
struct Grids {
    std::string file{};
    std::vector<std::string> names{};
};

/**
 * The grids RenderMan's `Volume` of type `blobbydso:impl_openvdb` reads. The file is the first of
 * its `blobbydso:stringargs`. The grids are the one they name second, before any `:` and the
 * class after it, then each grid a varying or vertex float parameter names.
 *
 * A grid that does not hold floats is logged and not read, as are a third string argument's
 * options. A grid of the `levelset` class, which gives no density, too few string arguments, and
 * a `Volume` of any other type, such as `"box"`, are logged and give nothing.
 **/
std::optional<Grids> openVdbGrids(const v3d::rib::requests::Volume & request, const v3d::rib::ParameterList & parameters, Warnings * warnings);

};  // namespace v3d::moya::ri::internal
