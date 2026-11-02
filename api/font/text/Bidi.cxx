/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bidi.h"

#include <api/font/text/internal/BidiHandle.h>
#include <api/font/text/internal/Icu.h>
#include <unicode/ubidi.h>
#include <unicode/utypes.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::font::text {

namespace {

/**
 * The paragraph levels of left to right and right to left text.
 **/
constexpr std::uint8_t LEFT_TO_RIGHT = 0;
constexpr std::uint8_t RIGHT_TO_LEFT = 1;

/**
 * @return the paragraph level ICU is given for a base direction
 **/
UBiDiLevel paragraphLevel(BaseDirection base) {
    switch (base) {
        case BaseDirection::LeftToRight:
            return LEFT_TO_RIGHT;
        case BaseDirection::RightToLeft:
            return RIGHT_TO_LEFT;
        case BaseDirection::Auto:
        default:
            return UBIDI_DEFAULT_LTR;
    }
}

/**
 * @return whether a level reads right to left
 **/
bool odd(std::uint8_t level) {
    return (level & 1U) != 0;
}

};  // namespace

Bidi::Bidi(std::string_view utf8, BaseDirection base) :
    handle_(boost::make_shared<internal::BidiHandle>(utf8, paragraphLevel(base))),
    size_(utf8.size()),
    base_(base == BaseDirection::RightToLeft ? RIGHT_TO_LEFT : LEFT_TO_RIGHT) {
    if (utf8.empty()) {
        return;
    }
    if (handle_->paragraph() == nullptr) {
        runs_.push_back({0, size_, base_});
        return;
    }
    const auto length = handle_->length();
    std::int32_t position = 0;
    while (position < length) {
        std::int32_t limit = length;
        UBiDiLevel found = 0;
        ubidi_getLogicalRun(handle_->paragraph(), position, &limit, &found);
        if (limit <= position) {
            break;
        }
        runs_.push_back({handle_->utf8(position), handle_->utf8(limit), found});
        position = limit;
    }
}

const std::vector<LevelRun>& Bidi::runs() const noexcept {
    return runs_;
}

std::uint8_t Bidi::level(std::size_t offset) const {
    if (handle_->paragraph() == nullptr) {
        return base_;
    }
    if (offset >= size_) {
        return rightToLeft(offset) ? RIGHT_TO_LEFT : LEFT_TO_RIGHT;
    }
    return ubidi_getLevelAt(handle_->paragraph(), handle_->utf16(offset));
}

bool Bidi::rightToLeft(std::size_t offset) const {
    if (handle_->paragraph() == nullptr) {
        return odd(base_);
    }
    const auto length = handle_->length();
    if (length == 0) {
        return odd(ubidi_getParaLevel(handle_->paragraph()));
    }
    const std::int32_t index = handle_->utf16(offset) < length ? handle_->utf16(offset) : length - 1;
    UBiDiLevel found = 0;
    UErrorCode status = U_ZERO_ERROR;
    ubidi_getParagraph(handle_->paragraph(), index, nullptr, nullptr, &found, &status);
    return !internal::failed(status) ? odd(found) : odd(base_);
}

std::vector<LevelRun> Bidi::line(std::size_t start, std::size_t end) const {
    std::vector<LevelRun> visual;
    if (start >= end || end > size_) {
        return visual;
    }
    if (handle_->paragraph() == nullptr) {
        visual.push_back({start, end, base_});
        return visual;
    }
    const std::int32_t first = handle_->utf16(start);
    const std::int32_t limit = handle_->utf16(end);
    UErrorCode status = U_ZERO_ERROR;
    const std::unique_ptr<UBiDi, decltype(&ubidi_close)> line(ubidi_openSized(0, 0, &status), &ubidi_close);
    ubidi_setLine(handle_->paragraph(), first, limit, line.get(), &status);
    const std::int32_t count = ubidi_countRuns(line.get(), &status);
    if (internal::failed(status)) {
        return visual;
    }
    for (std::int32_t run = 0; run < count; run++) {
        std::int32_t logical = 0;
        std::int32_t length = 0;
        ubidi_getVisualRun(line.get(), run, &logical, &length);
        visual.push_back({handle_->utf8(first + logical), handle_->utf8(first + logical + length), ubidi_getLevelAt(line.get(), logical)});
    }
    return visual;
}

CaretEdge Bidi::caret(std::size_t offset, Affinity affinity) const {
    if (size_ == 0) {
        return {0, rightToLeft(0)};
    }
    const std::int32_t index = handle_->utf16(offset < size_ ? offset : size_);
    const std::int32_t length = handle_->utf16(size_);
    const bool before = index > 0 && (affinity == Affinity::Upstream || index >= length);
    if (before) {
        // a reader's eye leaves a character by its end edge: its right when it reads left to right
        const std::size_t character = handle_->utf8(index - 1);
        return {character, !odd(level(character))};
    }
    const std::size_t character = handle_->utf8(index);
    return {character, odd(level(character))};
}

};  // namespace v3d::font::text
