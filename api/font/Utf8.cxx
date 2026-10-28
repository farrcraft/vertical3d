/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Utf8.h"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace v3d::font {

/**
 **/
std::u32string decode(std::string_view utf8) {
    std::u32string decoded;
    decoded.reserve(utf8.size());
    std::size_t index = 0;
    while (index < utf8.size()) {
        decoded.push_back(next(utf8, &index));
    }
    return decoded;
}

/**
 **/
char32_t next(std::string_view utf8, std::size_t* index) {
    const char32_t replacement = 0xFFFD;
    // a byte below this is a whole character on its own
    constexpr int singleByteLimit = 0x80;
    // the smallest code point each longer sequence may encode; anything below is overlong
    constexpr char32_t twoByteSmallest = 0x80;
    constexpr char32_t threeByteSmallest = 0x800;
    constexpr char32_t fourByteSmallest = 0x10000;
    // the same, indexed by sequence length
    constexpr std::array<char32_t, 5> minimum = {0, 0, twoByteSmallest, threeByteSmallest, fourByteSmallest};
    // a lead byte of each sequence length: the mask of its tag bits, the tag, and the mask of
    // the code point bits it carries
    constexpr int twoByteMask = 0xE0;
    constexpr int twoByteTag = 0xC0;
    constexpr int twoBytePayload = 0x1F;
    constexpr int threeByteMask = 0xF0;
    constexpr int threeByteTag = 0xE0;
    constexpr int threeBytePayload = 0x0F;
    constexpr int fourByteMask = 0xF8;
    constexpr int fourByteTag = 0xF0;
    constexpr int fourBytePayload = 0x07;
    // a continuation byte: the mask of its tag bits, the tag, and the six code point bits it
    // carries
    constexpr int continuationMask = 0xC0;
    constexpr int continuationTag = 0x80;
    constexpr int continuationBits = 6;
    constexpr int continuationPayload = 0x3F;
    // the largest code point, and the surrogate range, which no code point may fall in
    constexpr char32_t largestPoint = 0x10FFFF;
    constexpr char32_t firstSurrogate = 0xD800;
    constexpr char32_t lastSurrogate = 0xDFFF;

    const std::size_t start = *index;
    const unsigned char lead = static_cast<unsigned char>(utf8[start]);
    std::size_t length = 0;
    char32_t point = 0;
    if (lead < singleByteLimit) {
        length = 1;
        point = lead;
    } else if ((lead & twoByteMask) == twoByteTag) {
        length = 2;
        point = static_cast<char32_t>(lead & twoBytePayload);
    } else if ((lead & threeByteMask) == threeByteTag) {
        length = 3;
        point = static_cast<char32_t>(lead & threeBytePayload);
    } else if ((lead & fourByteMask) == fourByteTag) {
        length = 4;
        point = static_cast<char32_t>(lead & fourBytePayload);
    }

    bool valid = length > 0 && start + length <= utf8.size();
    for (std::size_t offset = 1; valid && offset < length; offset++) {
        const unsigned char following = static_cast<unsigned char>(utf8[start + offset]);
        if ((following & continuationMask) != continuationTag) {
            valid = false;
        } else {
            point = (point << continuationBits) | static_cast<char32_t>(following & continuationPayload);
        }
    }
    if (valid && (point < minimum.at(length) || point > largestPoint || (point >= firstSurrogate && point <= lastSurrogate))) {
        valid = false;
    }

    if (!valid) {
        *index = start + 1;
        return replacement;
    }
    *index = start + length;
    return point;
}

};  // namespace v3d::font
