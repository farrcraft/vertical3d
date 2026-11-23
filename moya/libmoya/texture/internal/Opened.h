/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <OpenEXR/ImfTiledInputFile.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::texture::internal {

/**
 * A texture file the cache reads from: the open file, the channels it reads in order, the size of
 * its tiles, and a lock that one thread holds while it reads a tile.
 **/
struct Opened {
    std::string path;
    boost::shared_ptr<Imf::TiledInputFile> file;
    std::vector<std::string> channels;
    std::uint32_t tileWidth { 0 };
    std::uint32_t tileHeight { 0 };
    std::mutex reading;
};

};  // namespace v3d::moya::texture::internal
