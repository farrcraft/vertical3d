/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Measure.h"

#include <api/font/ShapedGlyph.h>
#include <api/font/ShapedRun.h>
#include <api/font/ShapingCache.h>
#include <api/font/Utf8.h>
#include <unicode/uchar.h>
#include <unicode/umachine.h>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace v3d::font::internal {

Measure::Measure(const ShapedText& shaped) :
    shaped_(shaped),
    prefix_(shaped.text.size() + 1, 0.0F) {
    std::vector<float> advances(shaped.text.size(), 0.0F);
    for (const ShapedRun& run : shaped.runs) {
        for (const ShapedGlyph& glyph : run.glyphs) {
            if (glyph.cluster < advances.size()) {
                advances[glyph.cluster] += glyph.advance;
            }
        }
    }
    for (std::size_t index = 0; index < advances.size(); index++) {
        prefix_[index + 1] = prefix_[index] + advances[index];
    }
}

float Measure::width(std::size_t start, std::size_t end) const {
    return prefix_[end] - prefix_[start];
}

std::size_t Measure::visibleEnd(std::size_t start, std::size_t end) const {
    return trim(start, end, &Measure::separator);
}

std::size_t Measure::contentEnd(std::size_t start, std::size_t end) const {
    return trim(start, end, &Measure::white);
}

float Measure::contentWidth(std::size_t start, std::size_t end) const {
    return width(start, contentEnd(start, end));
}

std::size_t Measure::fit(std::size_t start, std::size_t end, float limit) const {
    const std::vector<std::size_t>& clusters = shaped_.graphemes;
    std::size_t fitted = start;
    for (auto at = std::upper_bound(clusters.begin(), clusters.end(), start); at != clusters.end() && *at <= end; ++at) {
        const bool over = width(start, *at) > limit;
        if (over && fitted != start) {
            break;
        }
        fitted = *at;
        if (over) {
            break;
        }
    }
    return fitted;
}

bool Measure::separator(char32_t charcode) {
    return charcode == U'\n' || charcode == U'\r' || charcode == U'\v' || charcode == U'\f' || charcode == NEXT_LINE ||
        charcode == LINE_SEPARATOR || charcode == PARAGRAPH_SEPARATOR;
}

bool Measure::white(char32_t charcode) {
    const bool holding = charcode == NO_BREAK_SPACE || charcode == FIGURE_SPACE || charcode == NARROW_NO_BREAK_SPACE;
    return separator(charcode) || (!holding && u_isUWhiteSpace(static_cast<UChar32>(charcode)) != 0);
}

std::size_t Measure::trim(std::size_t start, std::size_t end, bool (*test)(char32_t)) const {
    const std::vector<std::size_t>& clusters = shaped_.graphemes;
    auto at = std::lower_bound(clusters.begin(), clusters.end(), end);
    while (at != clusters.begin() && *(at - 1) >= start && *(at - 1) < end) {
        std::size_t index = *(at - 1);
        if (!test(next(shaped_.text, &index))) {
            break;
        }
        end = *(at - 1);
        --at;
    }
    return end;
}

};  // namespace v3d::font::internal
