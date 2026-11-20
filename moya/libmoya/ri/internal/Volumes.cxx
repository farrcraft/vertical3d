/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/ri/internal/Volumes.h>

#include <api/rib/Declaration.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace v3d::moya::ri::internal {

namespace {

/** The type of RenderMan's volume that reads its values from an OpenVDB file. **/
constexpr const char * kOpenVdb = "blobbydso:impl_openvdb";

};  // namespace

std::optional<Grids> openVdbGrids(const v3d::rib::requests::Volume & request, const v3d::rib::ParameterList & parameters, Warnings * warnings) {
    typedef v3d::rib::Declaration Declaration;
    if (request.type != kOpenVdb) {
        warnings->once("volume " + request.type, "a Volume of type \"" + request.type +
            "\" makes nothing, since moya's volumes read OpenVDB grids. A medium inside a shape is a volume's Bxdf given to the shape");
        return std::nullopt;
    }
    const std::vector<std::string> & arguments = parameters.strings("blobbydso:stringargs");
    if (arguments.size() < 2) {
        warnings->once("volume arguments", "an OpenVDB Volume names its file and its grid in blobbydso:stringargs, and one that does not makes nothing");
        return std::nullopt;
    }
    if (arguments.size() > 2) {
        warnings->once("volume options", "the options an OpenVDB Volume's third string argument gives are not read");
    }
    const std::size_t colon = arguments[1].find(':');
    const std::string grid = arguments[1].substr(0, colon);
    if (colon != std::string::npos && arguments[1].substr(colon + 1) == "levelset") {
        warnings->once("level set " + grid, "the grid \"" + grid + "\" is a level set, which gives no density, so its Volume makes nothing");
        return std::nullopt;
    }
    // the grid the arguments name, then each other grid a varying parameter names
    Grids out { .file = arguments[0], .names = { grid } };
    for (const std::string & name : parameters.names()) {
        const Declaration & declared = *parameters.declaration(name);
        if (declared.storage() != Declaration::Storage::VARYING && declared.storage() != Declaration::Storage::VERTEX) {
            continue;
        }
        if (declared.type() != Declaration::Type::FLOAT || declared.count() != 1) {
            warnings->once("grid " + name, "the grid \"" + name + "\" does not hold floats, and a volume reads floats alone, so it is not read");
            continue;
        }
        if (std::ranges::find(out.names, name) == out.names.end()) {
            out.names.push_back(name);
        }
    }
    return out;
}

};  // namespace v3d::moya::ri::internal
