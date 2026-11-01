/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Segmenter.h"

#include <api/font/internal/BreakHandle.h>
#include <api/font/internal/Icu.h>
#include <api/log/Logger.h>
#include <unicode/ubrk.h>
#include <unicode/utext.h>
#include <unicode/utypes.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Utf8.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::font {

namespace {

using Text = std::unique_ptr<UText, decltype(&utext_close)>;

/**
 * Point an iterator at a UTF-8 string, which it reads in place. Its offsets are then byte
 * offsets in the string.
 *
 * @return the text the iterator reads, which has to stay open while it is used, or null when it
 *         could not be opened
 **/
Text attach(UBreakIterator* iterator, std::string_view utf8) {
    UErrorCode status = U_ZERO_ERROR;
    Text text(utext_openUTF8(nullptr, utf8.data(), static_cast<std::int64_t>(utf8.size()), &status), &utext_close);
    if (internal::failed(status)) {
        return Text(nullptr, &utext_close);
    }
    ubrk_setUText(iterator, text.get(), &status);
    if (internal::failed(status)) {
        return Text(nullptr, &utext_close);
    }
    return text;
}

};  // namespace

Segmenter::Segmenter(const std::string& locale, const boost::shared_ptr<v3d::log::Logger>& logger) :
    handle_(boost::make_shared<internal::BreakHandle>(locale)) {
    if (!valid()) {
        logger->get()->error("The text segmentation rules for locale '{}' could not be loaded", locale);
    }
}

bool Segmenter::valid() const {
    return handle_->graphemes != nullptr && handle_->words != nullptr && handle_->lines != nullptr;
}

std::vector<std::size_t> Segmenter::graphemes(std::string_view utf8) const {
    std::vector<std::size_t> offsets{0};
    const Text text = valid() ? attach(handle_->graphemes, utf8) : Text(nullptr, &utext_close);
    if (!text) {
        std::size_t index = 0;
        while (index < utf8.size()) {
            next(utf8, &index);
            offsets.push_back(index);
        }
        return offsets;
    }
    for (std::int32_t offset = ubrk_next(handle_->graphemes); offset != UBRK_DONE; offset = ubrk_next(handle_->graphemes)) {
        offsets.push_back(static_cast<std::size_t>(offset));
    }
    return offsets;
}

std::vector<WordSegment> Segmenter::words(std::string_view utf8) const {
    std::vector<WordSegment> segments;
    if (utf8.empty()) {
        return segments;
    }
    const Text text = valid() ? attach(handle_->words, utf8) : Text(nullptr, &utext_close);
    if (!text) {
        segments.push_back({0, utf8.size(), false});
        return segments;
    }
    std::size_t start = 0;
    for (std::int32_t offset = ubrk_next(handle_->words); offset != UBRK_DONE; offset = ubrk_next(handle_->words)) {
        // the status is that of the rule that ended the segment just passed
        const bool word = ubrk_getRuleStatus(handle_->words) >= UBRK_WORD_NONE_LIMIT;
        segments.push_back({start, static_cast<std::size_t>(offset), word});
        start = static_cast<std::size_t>(offset);
    }
    return segments;
}

std::vector<LineBreak> Segmenter::lineBreaks(std::string_view utf8) const {
    std::vector<LineBreak> breaks;
    if (utf8.empty()) {
        return breaks;
    }
    const Text text = valid() ? attach(handle_->lines, utf8) : Text(nullptr, &utext_close);
    if (!text) {
        breaks.push_back({utf8.size(), false});
        return breaks;
    }
    for (std::int32_t offset = ubrk_next(handle_->lines); offset != UBRK_DONE; offset = ubrk_next(handle_->lines)) {
        const std::int32_t status = ubrk_getRuleStatus(handle_->lines);
        breaks.push_back({static_cast<std::size_t>(offset), status >= UBRK_LINE_HARD && status < UBRK_LINE_HARD_LIMIT});
    }
    return breaks;
}

};  // namespace v3d::font
