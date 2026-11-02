/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CurveText.h"

#include <api/font/CurveStore.h>
#include <api/font/Paragraph.h>
#include <api/font/ShapedGlyph.h>
#include <api/render/realtime/canvas/Canvas.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#include "CurvePages.h"

#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::paint {

namespace {

/**
 * The corners of a quad in the order the canvas winds them: top left, top right, bottom right,
 * bottom left, with y upwards in the em square. Each says which of the outline's two corners it
 * takes on x and on y.
 **/
constexpr std::array<glm::bvec2, 4> CORNERS{ glm::bvec2(false, true), glm::bvec2(true, true), glm::bvec2(true, false), glm::bvec2(false, false) };

/**
 * The two triangles over a quad's four corners.
 **/
constexpr std::array<std::uint32_t, 6> TRIANGLES{ 0, 1, 2, 2, 3, 0 };

};  // namespace

/**
 **/
CurveText::CurveText(CurvePages pages) :
    pages_(std::move(pages)) {
}

/**
 **/
void CurveText::draw(render::realtime::canvas::Canvas* canvas, const font::Paragraph& paragraph, const glm::mat3& transform) {
    if (canvas == nullptr) {
        return;
    }
    // a pixel of the canvas, in the paragraph's pixels, from how much the transform scales area
    const float scale = std::sqrt(std::abs(glm::determinant(glm::mat2(transform))));
    vertices_.clear();
    indices_.clear();
    for (const font::Line& line : paragraph.lines()) {
        for (const font::LineRun& run : line.runs) {
            if (scale <= 0.0F || run.size <= 0.0F || !std::isfinite(scale)) {
                continue;
            }
            const float margin = 1.0F / (run.size * scale);
            glm::vec2 pen(run.x, line.baseline);
            for (const font::ShapedGlyph& glyph : run.glyphs) {
                const font::StoredCurves& stored = store_.glyph(*run.face, glyph.index);
                const glm::vec2 origin = pen + glyph.offset;
                pen.x += glyph.advance;
                if (!stored.empty) {
                    const glm::vec4& colour = paragraph.styleAt(run.ellipsis ? run.start : glyph.cluster).colour;
                    quad(stored, Placing{ .origin = origin, .size = run.size, .margin = margin, .transform = transform }, colour);
                }
            }
        }
    }
    // the curves a glyph was just read into have to be on the device before a quad names them
    pages_.sync(&store_);
    if (!vertices_.empty() && pages_.curves().valid() && pages_.bands().valid()) {
        canvas->curves(vertices_, indices_, pages_.curves(), pages_.bands());
    }
}

/**
 **/
void CurveText::quad(const font::StoredCurves& stored, const Placing& placing, const glm::vec4& colour) {
    const auto first = static_cast<std::uint32_t>(vertices_.size());
    const float margin = placing.margin;
    for (const glm::bvec2& corner : CORNERS) {
        const glm::vec2 em(corner.x ? stored.max.x + margin : stored.min.x - margin, corner.y ? stored.max.y + margin : stored.min.y - margin);
        const glm::vec3 placed = placing.transform * glm::vec3(placing.origin + (glm::vec2(em.x, -em.y) * placing.size), 1.0F);
        vertices_.push_back({ .position = glm::vec2(placed), .uv = em, .colour = colour, .glyph = stored.bands });
    }
    for (const std::uint32_t index : TRIANGLES) {
        indices_.push_back(first + index);
    }
}

/**
 **/
const font::CurveStore& CurveText::store() const noexcept {
    return store_;
}

};  // namespace v3d::ui::paint
