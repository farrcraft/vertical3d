/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/texture/Texture.h>
#include <moya/libmoya/texture/internal/Tile.h>
#include <moya/libmoya/texture/internal/TileKey.h>

#include <cstddef>
#include <cstdint>
#include <list>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::texture {

namespace internal {
struct Opened;
};  // namespace internal

/**
 * The texture files a render reads, and the tiles of them it holds in memory, within a limit of
 * bytes. A texel's tile is loaded when the cache does not hold it, and the tile used longest ago
 * is dropped when the tiles held pass the limit. The tile just loaded is never dropped, so a
 * limit below one tile holds one. A texel's value is the file's whatever the cache holds.
 *
 * Every tile is held as four floats for each texel, as kernel::texture::expanded() gives them.
 * Any number of threads may read texels at once. A thread loading a tile holds only its file's
 * lock while it reads.
 **/
class Cache final {
 public:
    /** @param bytes the most the tiles held take, as four floats for each texel **/
    explicit Cache(std::size_t bytes);
    Cache(const Cache &) = delete;
    Cache & operator=(const Cache &) = delete;

    /**
     * Opens a texture file, a tiled OpenEXR file of every level rounded down, as texture::make()
     * writes one, and gives its number. It reads the file's R, G, B and A, or its Y and A.
     *
     * @param path the file, as UTF-8
     * @throws std::runtime_error when the file is not such a texture, or holds neither R, G and B
     *         nor Y
     * @throws std::exception, from OpenEXR, when it cannot be read
     **/
    std::uint32_t open(const std::string & path);

    /** Each texture's size, by its number. **/
    const std::vector<kernel::texture::Texture> & textures() const {
        return textures_;
    }

    /**
     * Writes one texel's four channels to out, loading its tile when the cache does not hold it.
     * A texel that cannot be read is zero, and its file is named among failures().
     *
     * @param cache the Cache, as device::cpu::Textures holds it
     **/
    static void fetch(void * cache, std::uint32_t texture, std::uint32_t level, std::uint32_t column, std::uint32_t row, float * out) noexcept;

    /** A line for each file a texel could not be read from, saying why. **/
    std::vector<std::string> failures() const;

    /** How many tiles have been loaded, counting each time a tile is loaded again. **/
    std::uint64_t loads() const;

 private:
    /** The texel, loading its tile when needed. **/
    void read(std::uint32_t texture, std::uint32_t level, std::uint32_t column, std::uint32_t row, float * out);

    /** A tile read from its file. **/
    internal::Tile load(const internal::TileKey & key);

    /** Copies a texel of a held tile into out, and marks the tile used, with mutex_ held. **/
    void copyOut(internal::Tile & tile, std::uint32_t column, std::uint32_t row, float * out);

    std::size_t limit_;
    std::vector<boost::shared_ptr<internal::Opened>> files_;
    std::vector<kernel::texture::Texture> textures_;

    mutable std::mutex mutex_;
    std::unordered_map<internal::TileKey, internal::Tile, internal::TileKey::Hash> tiles_;
    std::list<internal::TileKey> used_;
    std::size_t held_ { 0 };
    std::uint64_t loads_ { 0 };
    std::set<std::string> failures_;
};

};  // namespace v3d::moya::texture
