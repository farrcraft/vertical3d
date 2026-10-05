/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

namespace v3d::render::offline {

/**
 * The directories an `Option "searchpath"` names, with `&` replaced by what the path was
 * before, which is how a scene appends to a path rather than replacing what a driver put there.
 *
 * RI separates directories with a colon, which is also what a Windows drive letter is
 * followed by, so a lone letter before one does not end a directory.
 **/
std::vector<std::string> searchpath(const std::string & path, const std::vector<std::string> & before);

};  // namespace v3d::render::offline
