/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <unicode/ubidi.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace v3d::font::text::internal {

/**
 * Owns ICU's resolution of one text's embedding levels, from construction to destruction, with
 * the UTF-16 copy of the text it reads and the maps between its offsets and UTF-8's.
 **/
class BidiHandle final {
 public:
    BidiHandle(std::string_view utf8, UBiDiLevel base);
    ~BidiHandle();
    BidiHandle(const BidiHandle&) = delete;
    BidiHandle& operator=(const BidiHandle&) = delete;

    /**
     * @return the UTF-16 index of the character holding a byte offset, or the UTF-16 length at
     *         or past the end
     **/
    std::int32_t utf16(std::size_t offset) const;

    /**
     * @return the byte offset where the character holding a UTF-16 index starts, or the byte
     *         length at or past the end
     **/
    std::size_t utf8(std::int32_t index) const;

    /**
     * @return the resolved paragraphs, or null when the levels could not be resolved
     **/
    UBiDi* paragraph() const noexcept;

    /**
     * @return the text's length in UTF-16 units
     **/
    std::int32_t length() const noexcept;

 private:
    UBiDi* paragraph_ = nullptr;
    std::u16string text_;
    std::vector<std::int32_t> toUtf16_;  // one entry for each byte, and one for the end
    std::vector<std::size_t> toUtf8_;    // one entry for each UTF-16 unit, and one for the end
};

};  // namespace v3d::font::text::internal
