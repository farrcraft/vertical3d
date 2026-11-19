/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/output/Cryptomatte.h>

#include <moya/libmoya/output/Channel.h>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace v3d::moya::output {

namespace {

/** MurmurHash3's constants for 32 bits. **/
constexpr std::uint32_t kFirst = 0xcc9e2d51U;
constexpr std::uint32_t kSecond = 0x1b873593U;
constexpr std::uint32_t kStep = 0xe6546b64U;
constexpr std::uint32_t kMixFirst = 0x85ebca6bU;
constexpr std::uint32_t kMixSecond = 0xc2b2ae35U;
constexpr std::uint32_t kFive = 5;
constexpr int kBlockTurn = 15;
constexpr int kHashTurn = 13;
constexpr std::uint32_t kFoldMiddle = 13;

/** A float's exponent: its place in the bits, and the mask of its eight bits. **/
constexpr std::uint32_t kExponentShift = 23;
constexpr std::uint32_t kExponentMask = 0xffU;

/** A block MurmurHash3 reads at once, in bytes. **/
constexpr std::size_t kBlock = 4;

/** A 32-bit word's hex digits. **/
constexpr std::size_t kHexDigits = 8;

/** The hex digits of a layer's key. **/
constexpr std::size_t kKeyDigits = 7;

/** A control character's JSON escape, \u and four hex digits, with its end. **/
constexpr std::size_t kEscaped = 7;

/** MurmurHash3's mix of a block before it is folded in. **/
std::uint32_t scrambled(std::uint32_t block) {
    block *= kFirst;
    block = std::rotl(block, kBlockTurn);
    return block * kSecond;
}

/** A word as eight lower-case hex digits. **/
std::string hexOf(std::uint32_t word) {
    char text[kHexDigits + 1] {};
    std::snprintf(text, sizeof(text), "%08x", word);
    return std::string(text, kHexDigits);
}

/** A text as a JSON string, its quotes and backslashes escaped, and its controls as \u codes. **/
std::string quoted(std::string_view text) {
    std::string out = "\"";
    for (const char letter : text) {
        const auto code = static_cast<unsigned char>(letter);
        if (letter == '"' || letter == '\\') {
            out += '\\';
            out += letter;
        } else if (code < 0x20U) {
            char escaped[kEscaped] {};
            std::snprintf(escaped, sizeof(escaped), "\\u%04x", code);
            out += escaped;
        } else {
            out += letter;
        }
    }
    return out + "\"";
}

};  // namespace

std::uint32_t murmur3(std::string_view text, std::uint32_t seed) {
    std::uint32_t hash = seed;
    const std::size_t blocks = text.size() / kBlock;
    for (std::size_t i = 0; i < blocks; ++i) {
        std::uint32_t block = 0;
        for (std::size_t byte = 0; byte < kBlock; ++byte) {
            block |= static_cast<std::uint32_t>(static_cast<unsigned char>(text[(i * kBlock) + byte])) << (8U * byte);
        }
        hash ^= scrambled(block);
        hash = std::rotl(hash, kHashTurn);
        hash = (hash * kFive) + kStep;
    }
    std::uint32_t tail = 0;
    const std::size_t left = text.size() % kBlock;
    for (std::size_t byte = left; byte-- > 0;) {
        tail = (tail << 8U) | static_cast<unsigned char>(text[(blocks * kBlock) + byte]);
    }
    if (left != 0) {
        hash ^= scrambled(tail);
    }
    hash ^= static_cast<std::uint32_t>(text.size());
    hash ^= hash >> 16U;
    hash *= kMixFirst;
    hash ^= hash >> kFoldMiddle;
    hash *= kMixSecond;
    hash ^= hash >> 16U;
    return hash;
}

std::uint32_t idOf(std::string_view name) {
    std::uint32_t hash = murmur3(name);
    const std::uint32_t exponent = (hash >> kExponentShift) & kExponentMask;
    if (exponent == 0U || exponent == kExponentMask) {
        hash ^= 1U << kExponentShift;
    }
    return hash;
}

float floatOf(std::uint32_t id) {
    return std::bit_cast<float>(id);
}

std::string keyOf(std::string_view layer) {
    return hexOf(murmur3(layer)).substr(0, kKeyDigits);
}

std::optional<Cryptomatte> cryptomatteOf(const Channel & channel) {
    if (channel.source != Source::Raw) {
        return std::nullopt;
    }
    Cryptomatte out { .name = channel.film.name, .names = Cryptomatte::Names::Object, .ranks = std::max(channel.ranks, 1U) };
    if (channel.sourceName == "crypto_material") {
        out.names = Cryptomatte::Names::Material;
    } else if (channel.sourceName == "crypto_asset") {
        out.names = Cryptomatte::Names::Asset;
    } else if (channel.sourceName != "crypto_object") {
        return std::nullopt;
    }
    return out;
}

std::vector<std::string> rankChannelsOf(const Cryptomatte & layer) {
    std::vector<std::string> out;
    const std::uint32_t channels = (layer.ranks + kRanksPerChannel - 1U) / kRanksPerChannel;
    for (std::uint32_t i = 0; i < channels; ++i) {
        char number[11] {};
        std::snprintf(number, sizeof(number), "%02u", i);
        out.push_back(layer.name + number);
    }
    return out;
}

std::string manifestOf(const std::vector<std::string> & names) {
    std::vector<std::string> sorted = names;
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    std::string out = "{";
    for (const std::string & name : sorted) {
        out += (out.size() > 1U ? "," : "") + quoted(name) + ":" + quoted(hexOf(idOf(name)));
    }
    return out + "}";
}

};  // namespace v3d::moya::output
