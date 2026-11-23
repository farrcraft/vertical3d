/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace v3d::moya::scene {
class Scene;
};  // namespace v3d::moya::scene

namespace v3d::moya::texture {

/** The bytes in a kilobyte, which RI's texturememory limit is given in. **/
inline constexpr std::size_t kKilobyte = 1024;

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

    bool operator==(const Options &) const = default;
};

/**
 * The options a scene's options give: `textureSearchPath` as directories, `textureMemory` in
 * kilobytes, and `textureDirectory`, or their fallbacks while the scene has no options.
 **/
Options optionsOf(const scene::Scene & scene);

};  // namespace v3d::moya::texture
