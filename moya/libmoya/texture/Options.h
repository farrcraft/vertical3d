/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace v3d::moya::texture {

/** The most bytes of tiles a render holds when nothing names a limit: 1 GiB. **/
inline constexpr std::size_t kDefaultMemory = std::size_t { 1024 } * 1024 * 1024;

/**
 * Where a render finds and makes its textures, and how much of them it holds.
 *
 * - searchPath holds the directories a relative path is looked for in, in order. With none, it is
 *   looked for in the folder moya runs from.
 * - memory is the most bytes the tiles held take.
 * - directory is where an image that is not a texture is made into one. Empty makes it in
 *   `moya-textures` in the system's temporary directory.
 **/
struct Options {
    std::vector<std::string> searchPath;
    std::size_t memory { kDefaultMemory };
    std::string directory;
};

};  // namespace v3d::moya::texture
