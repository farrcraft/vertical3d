/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "BreakHandle.h"

#include <api/font/text/internal/Icu.h>
#include <unicode/ubrk.h>
#include <unicode/uloc.h>
#include <unicode/utypes.h>

#include <array>
#include <string>

namespace v3d::font::text::internal {

namespace {

/**
 * The longest ICU locale ID a language tag is turned into.
 **/
constexpr int LOCALE_CAPACITY = ULOC_FULLNAME_CAPACITY;

/**
 * @return an iterator of a kind in a locale, or null when its rules do not load
 **/
UBreakIterator* opened(UBreakIteratorType type, const char* locale) {
    UErrorCode status = U_ZERO_ERROR;
    UBreakIterator* iterator = ubrk_open(type, locale, nullptr, 0, &status);
    if (failed(status)) {
        ubrk_close(iterator);
        return nullptr;
    }
    return iterator;
}

};  // namespace

BreakHandle::BreakHandle(const std::string& locale) {
    // ICU names its locales by its own IDs, so a language tag such as "en-US" is turned into one
    std::array<char, LOCALE_CAPACITY> id{};
    UErrorCode status = U_ZERO_ERROR;
    uloc_forLanguageTag(locale.c_str(), id.data(), LOCALE_CAPACITY, nullptr, &status);
    if (failed(status) || status == U_STRING_NOT_TERMINATED_WARNING) {
        id.fill('\0');
    }
    graphemes = opened(UBRK_CHARACTER, id.data());
    words = opened(UBRK_WORD, id.data());
    lines = opened(UBRK_LINE, id.data());
}

BreakHandle::~BreakHandle() {
    ubrk_close(graphemes);
    ubrk_close(words);
    ubrk_close(lines);
}

};  // namespace v3d::font::text::internal
