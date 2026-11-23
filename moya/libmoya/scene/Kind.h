/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Declaration.h>

#include <string>
#include <vector>

namespace v3d::moya::scene {

/**
 * A kind of scene object, such as a camera or a mesh: its name and its declared parameters. A
 * kind that holds primvars takes a declaration of each one from its client, per object.
 **/
struct Kind {
    std::string name;
    std::vector<Declaration> parameters;
    bool primvars { false };
};

};  // namespace v3d::moya::scene
