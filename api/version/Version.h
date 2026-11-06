/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string_view>

namespace v3d::version {

/**
 * A version's three numbers. While major is 0, a minor release may break the api, and a patch
 * release never does.
 **/
struct Number final {
    /** Moves when a release breaks the api, once the api is stable. **/
    unsigned int major;
    /** Moves when a release adds to the api, or, while major is 0, breaks it. **/
    unsigned int minor;
    /** Moves when a release only fixes what an earlier one shipped. **/
    unsigned int patch;
};

/**
 * @return the numbers of the release the tree is building toward, from the root's project()
 **/
Number number() noexcept;

/**
 * The version to show a user, as a semantic version.
 *
 * A release build shows the bare number, such as 0.1.0. Any other build marks it as a
 * development build and adds the commit it was built from, as git describe names it, such as
 * 0.1.0-dev+v0.1.0-4-g1a2b3c4d-dirty. A tree that is not a git checkout gives 0.1.0-dev.
 *
 * @return the version, which lives as long as the program
 **/
std::string_view text() noexcept;

};  // namespace v3d::version
