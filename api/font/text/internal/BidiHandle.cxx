/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "BidiHandle.h"

#include <api/font/text/Utf8.h>
#include <api/font/text/internal/Icu.h>
#include <unicode/ubidi.h>
#include <unicode/utypes.h>

#include <climits>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace v3d::font::text::internal {

namespace {

/**
 * The first code point UTF-16 writes as a surrogate pair, and the bits each half carries.
 **/
constexpr char32_t SUPPLEMENTARY = 0x10000;
constexpr char32_t HIGH_SURROGATE = 0xD800;
constexpr char32_t LOW_SURROGATE = 0xDC00;
constexpr unsigned int SURROGATE_BITS = 10;
constexpr char32_t SURROGATE_MASK = 0x3FF;

};  // namespace

BidiHandle::BidiHandle(std::string_view utf8, UBiDiLevel base) {
    toUtf16_.reserve(utf8.size() + 1);
    std::size_t index = 0;
    while (index < utf8.size()) {
        const std::size_t start = index;
        const char32_t charcode = next(utf8, &index);
        const auto position = static_cast<std::int32_t>(text_.size());
        toUtf16_.insert(toUtf16_.end(), index - start, position);
        if (charcode < SUPPLEMENTARY) {
            text_.push_back(static_cast<char16_t>(charcode));
            toUtf8_.push_back(start);
        } else {
            const char32_t bits = charcode - SUPPLEMENTARY;
            text_.push_back(static_cast<char16_t>(HIGH_SURROGATE + (bits >> SURROGATE_BITS)));
            text_.push_back(static_cast<char16_t>(LOW_SURROGATE + (bits & SURROGATE_MASK)));
            toUtf8_.insert(toUtf8_.end(), 2, start);
        }
    }
    toUtf16_.push_back(static_cast<std::int32_t>(text_.size()));
    toUtf8_.push_back(utf8.size());

    if (text_.size() > static_cast<std::size_t>(INT32_MAX)) {
        return;
    }
    UErrorCode status = U_ZERO_ERROR;
    paragraph_ = ubidi_openSized(static_cast<std::int32_t>(text_.size()), 0, &status);
    // the text is not copied, so it stays in this handle for as long as the paragraph is used
    ubidi_setPara(paragraph_, text_.data(), static_cast<std::int32_t>(text_.size()), base, nullptr, &status);
    if (failed(status)) {
        ubidi_close(paragraph_);
        paragraph_ = nullptr;
    }
}

BidiHandle::~BidiHandle() {
    ubidi_close(paragraph_);
}

UBiDi* BidiHandle::paragraph() const noexcept {
    return paragraph_;
}

std::int32_t BidiHandle::length() const noexcept {
    return static_cast<std::int32_t>(text_.size());
}

std::int32_t BidiHandle::utf16(std::size_t offset) const {
    return offset < toUtf16_.size() ? toUtf16_[offset] : toUtf16_.back();
}

std::size_t BidiHandle::utf8(std::int32_t index) const {
    return index >= 0 && static_cast<std::size_t>(index) < toUtf8_.size() ? toUtf8_[static_cast<std::size_t>(index)] : toUtf8_.back();
}

};  // namespace v3d::font::text::internal
