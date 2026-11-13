/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace v3d::moya::texture::internal {

/** One tile of one level of a texture, by the tile's column and row from the level's top left. **/
struct TileKey {
    std::uint32_t texture { 0 };
    std::uint32_t level { 0 };
    std::uint32_t column { 0 };
    std::uint32_t row { 0 };

    bool operator==(const TileKey &) const = default;

    /** A hash of every field, for an unordered map of tiles. **/
    struct Hash {
        /** The prime the hash so far is multiplied by before each field's is folded in. **/
        static constexpr std::size_t kFold = 1000003U;

        std::size_t operator()(const TileKey & key) const {
            std::size_t out = std::hash<std::uint32_t> {}(key.texture);
            for (const std::uint32_t field : { key.level, key.column, key.row }) {
                out = (out * kFold) ^ std::hash<std::uint32_t> {}(field);
            }
            return out;
        }
    };
};

};  // namespace v3d::moya::texture::internal
