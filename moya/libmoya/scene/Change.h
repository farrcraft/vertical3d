/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Handle.h>

#include <cstdint>
#include <string>
#include <vector>

namespace v3d::moya::scene {

/** What happened to one object after a version. **/
struct Change {
    enum class What : std::uint8_t {
        Created,
        Modified,
        Removed
    };

    Handle handle;
    What what { What::Modified };

    /** The parameters set or declared after the version, by name, for a modified object. **/
    std::vector<std::string> parameters;
};

};  // namespace v3d::moya::scene
