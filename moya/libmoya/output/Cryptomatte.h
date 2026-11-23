/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/output/Channel.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace v3d::moya::output {

/** The ranks a Cryptomatte layer writes unless its channel asks for more. **/
inline constexpr std::uint32_t kRanks = 6;

/** The ranks each channel of a layer holds: two, an ID and its coverage in each. **/
inline constexpr std::uint32_t kRanksPerChannel = 2;

/**
 * A Cryptomatte layer: its name, what its IDs name, and how many of each pixel's IDs it keeps,
 * those of the most coverage. An object's ID names the innermost instance that places the
 * surface, a material's names the instance's material, and an asset's names the group the
 * outermost instance places, or that instance when it places a geometry.
 **/
struct Cryptomatte {
    enum class Names : std::uint8_t {
        Object,
        Material,
        Asset
    };

    std::string name;
    Names names { Names::Object };
    std::uint32_t ranks { kRanks };
};

/** The 32-bit MurmurHash3 of a text, from a seed, as Cryptomatte hashes a name. **/
std::uint32_t murmur3(std::string_view text, std::uint32_t seed = 0);

/**
 * A name's ID, as Cryptomatte stores it: the name's MurmurHash3. The lowest bit of its exponent
 * is flipped when the exponent is all zeros or all ones, so the float it is read as is neither a
 * NaN, an infinity nor a denormal.
 **/
std::uint32_t idOf(std::string_view name);

/** The float an ID is written as, which holds the ID's bits. **/
float floatOf(std::uint32_t id);

/** A layer's key in the file's header: the first seven hex digits of its name's MurmurHash3. **/
std::string keyOf(std::string_view layer);

/**
 * The layer a channel asks for: a raw channel of the source name crypto_object, crypto_material
 * or crypto_asset, named as its channel is. None for any other channel.
 **/
std::optional<Cryptomatte> cryptomatteOf(const Channel & channel);

/**
 * The film channels a layer's ranks are written to: the layer's name followed by 00, 01 and on,
 * two ranks to a channel of four components, ID then coverage in each pair.
 **/
std::vector<std::string> rankChannelsOf(const Cryptomatte & layer);

/** A layer's manifest: a JSON object of each name and its ID, as eight hex digits. **/
std::string manifestOf(const std::vector<std::string> & names);

};  // namespace v3d::moya::output
