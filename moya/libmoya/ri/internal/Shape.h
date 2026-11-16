/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/rib/Parameters.h>
#include <moya/libmoya/scene/Value.h>

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace v3d::moya::ri::internal {

/**
 * A primitive as the scene holds it: its kind, the request that made it, its parameters, and
 * the parameters of the request that become primvars. The names in `taken` are read into the
 * parameters, and do not become primvars.
 **/
struct Shape {
    std::string kind{};
    std::string request{};
    std::vector<std::pair<std::string, scene::Value>> parameters{};
    v3d::rib::ParameterList primvars{};
    std::set<std::string> taken{};
};

};  // namespace v3d::moya::ri::internal
