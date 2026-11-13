/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace v3d::moya::texture {

/**
 * The directories a search path names, in order. Directories are separated by a colon or a
 * semicolon, as RI's searchpath option separates them. A colon after a single letter at a
 * directory's start belongs to a drive, as in `C:/textures`. An empty directory is left out.
 **/
std::vector<std::string> directoriesOf(const std::string & path);

/**
 * Where a file a texture names is: the name itself when it is absolute, or the first directory
 * that holds it. A relative name is looked for in the folder moya runs from when no directory
 * is given. Nothing when no such file exists.
 *
 * @param name the file, as UTF-8
 **/
std::optional<std::string> found(const std::string & name, const std::vector<std::string> & directories);

};  // namespace v3d::moya::texture
