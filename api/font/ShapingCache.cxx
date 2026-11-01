/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ShapingCache.h"

#include <api/log/Logger.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Bidi.h"
#include "Feature.h"
#include "ShapedRun.h"
#include "Shaper.h"
#include "TextStyle.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::font {

namespace {

/**
 * @return the ranges a text is shaped in: those given when they cover it in order, else the
 *         first one's style over all of it, with adjacent ranges that shape alike joined
 **/
std::vector<StyledRange> shapingRanges(std::size_t size, const std::vector<StyledRange>& ranges) {
    bool covers = !ranges.empty() && ranges.front().start == 0 && ranges.back().end == size;
    for (std::size_t range = 0; covers && range < ranges.size(); range++) {
        covers = ranges[range].start <= ranges[range].end && (range == 0 || ranges[range].start == ranges[range - 1].end);
    }
    if (!covers) {
        return { StyledRange{ .start = 0, .end = size, .style = ranges.empty() ? TextStyle() : ranges.front().style } };
    }
    std::vector<StyledRange> joined;
    for (const StyledRange& range : ranges) {
        if (range.start == range.end) {
            continue;
        }
        if (!joined.empty() && joined.back().style.shapesLike(range.style)) {
            joined.back().end = range.end;
        } else {
            joined.push_back(range);
        }
    }
    return joined;
}

/**
 * @return the key a text is kept under: everything that decides its glyphs
 **/
std::string keyOf(std::string_view text, const std::vector<StyledRange>& ranges, BaseDirection base) {
    std::string key(text);
    key.push_back('\0');
    key += std::to_string(static_cast<int>(base));
    for (const StyledRange& range : ranges) {
        const TextStyle& style = range.style;
        key += '|' + std::to_string(range.start) + ',' + std::to_string(range.end) + ',' +
            std::to_string(reinterpret_cast<std::uintptr_t>(style.fonts.get())) + ',' + std::to_string(style.face.weight) + ',' +
            std::to_string(static_cast<int>(style.face.italic)) + ',' + std::to_string(std::bit_cast<std::uint32_t>(style.size));
        for (const Feature& feature : style.features) {
            key += ',' + feature.tag + '=' + std::to_string(feature.value);
        }
    }
    return key;
}

};  // namespace

ShapingCache::ShapingCache(const std::string& locale, const boost::shared_ptr<v3d::log::Logger>& logger, std::size_t capacity) :
    segmenter_(locale, logger),
    capacity_(capacity > 0 ? capacity : 1) {
}

boost::shared_ptr<const ShapedText> ShapingCache::shape(std::string_view text, const std::vector<StyledRange>& ranges, BaseDirection base) {
    std::vector<StyledRange> joined = shapingRanges(text.size(), ranges);
    std::string key = keyOf(text, joined, base);
    const auto found = entries_.find(key);
    if (found != entries_.end()) {
        order_.splice(order_.begin(), order_, found->second.used);
        return found->second.text;
    }

    const boost::shared_ptr<ShapedText> made = boost::make_shared<ShapedText>(ShapedText{ .text = std::string(text), .bidi = Bidi(text, base) });
    for (std::size_t range = 0; range < joined.size(); range++) {
        const StyledRange& styled = joined[range];
        const Shaper shaper(styled.style.fonts);
        for (ShapedRun& run : shaper.shape(made->text, styled.start, styled.end, made->bidi, styled.style.face, styled.style.size, styled.style.features)) {
            made->runs.push_back(std::move(run));
            made->runRanges.push_back(range);
        }
    }
    made->ranges = std::move(joined);
    made->graphemes = segmenter_.graphemes(made->text);
    made->breaks = segmenter_.lineBreaks(made->text);
    shaped_++;

    order_.push_front(key);
    entries_.emplace(std::move(key), Entry{ .text = made, .used = order_.begin() });
    while (entries_.size() > capacity_) {
        entries_.erase(order_.back());
        order_.pop_back();
    }
    return made;
}

std::size_t ShapingCache::shaped() const noexcept {
    return shaped_;
}

std::size_t ShapingCache::size() const noexcept {
    return entries_.size();
}

};  // namespace v3d::font
