/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Curves.h"

#include <api/font/internal/FaceHandle.h>
#include <api/font/internal/OutlineReader.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "Face.h"

#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::font {

namespace {

/**
 * The most bands an axis is divided into.
 **/
constexpr std::uint32_t MOST_BANDS = 16;

/**
 * @return a curve's lowest and highest coordinate on one axis
 **/
glm::vec2 extent(const QuadraticCurve& curve, int axis) {
    return glm::vec2(std::min({ curve.p1[axis], curve.p2[axis], curve.p3[axis] }), std::max({ curve.p1[axis], curve.p2[axis], curve.p3[axis] }));
}

/**
 * @param span the lowest and highest coordinate the bands cover on the axis
 * @return the bands of one axis: for each strip across it, the curves reaching into it, sorted
 *         from the largest coordinate on the other axis down. A line along the axis is in none
 **/
std::vector<std::vector<std::uint32_t>> band(const std::vector<QuadraticCurve>& curves, int axis, const glm::vec2& span, std::uint32_t count) {
    std::vector<std::vector<std::uint32_t>> bands(count);
    const int across = 1 - axis;
    const float thickness = (span.y - span.x) / static_cast<float>(count);
    for (std::uint32_t strip = 0; strip < count; strip++) {
        const float from = span.x + (thickness * static_cast<float>(strip)) - CurveGlyph::BAND_OVERLAP;
        const float to = span.x + (thickness * static_cast<float>(strip + 1)) + CurveGlyph::BAND_OVERLAP;
        std::vector<std::uint32_t>& held = bands[strip];
        for (std::uint32_t index = 0; index < curves.size(); index++) {
            const glm::vec2 reach = extent(curves[index], axis);
            if (reach.x != reach.y && reach.y >= from && reach.x <= to) {
                held.push_back(index);
            }
        }
        std::sort(held.begin(), held.end(), [&curves, across](std::uint32_t one, std::uint32_t other) {
            return extent(curves[one], across).y > extent(curves[other], across).y;
        });
    }
    return bands;
}

};  // namespace

glm::vec4 CurveGlyph::bandTransform() const {
    const glm::vec2 size = max - min;
    const glm::vec2 counts(static_cast<float>(vertical.size()), static_cast<float>(horizontal.size()));
    const glm::vec2 scale(size.x > 0.0F ? counts.x / size.x : 0.0F, size.y > 0.0F ? counts.y / size.y : 0.0F);
    return glm::vec4(scale, -min * scale);
}

CurveGlyph outline(const Face& face, unsigned int index, const std::vector<std::int32_t>& coordinates) {
    CurveGlyph glyph;
    FT_Face loaded = face.handle().face;
    if (loaded == nullptr || loaded->units_per_EM == 0 || !face.handle().vary(coordinates)) {
        return glyph;
    }
    // unscaled, so in design units, and with no transform a size left behind
    FT_Set_Transform(loaded, nullptr, nullptr);
    if (FT_Load_Glyph(loaded, index, FT_LOAD_NO_SCALE) != 0 || loaded->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
        return glyph;
    }
    internal::OutlineReader reader(static_cast<float>(loaded->units_per_EM));
    return banded(reader.read(&loaded->glyph->outline));
}

CurveGlyph banded(std::vector<QuadraticCurve> curves) {
    CurveGlyph glyph;
    glyph.curves = std::move(curves);
    if (glyph.curves.empty()) {
        return glyph;
    }

    glyph.min = glyph.curves.front().p1;
    glyph.max = glyph.min;
    for (const QuadraticCurve& curve : glyph.curves) {
        glyph.min = glm::min(glm::min(glyph.min, curve.p1), glm::min(curve.p2, curve.p3));
        glyph.max = glm::max(glm::max(glyph.max, curve.p1), glm::max(curve.p2, curve.p3));
    }
    const double root = std::sqrt(static_cast<double>(glyph.curves.size()));
    const auto count = std::clamp(static_cast<std::uint32_t>(std::lround(root)), std::uint32_t{1}, MOST_BANDS);  // checked: root of a count, so finite
    glyph.horizontal = band(glyph.curves, 1, glm::vec2(glyph.min.y, glyph.max.y), count);
    glyph.vertical = band(glyph.curves, 0, glm::vec2(glyph.min.x, glyph.max.x), count);
    return glyph;
}

};  // namespace v3d::font
