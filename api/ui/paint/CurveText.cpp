/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CurveText.h"

#include <api/font/CurveStore.h>
#include <api/font/Paragraph.h>
#include <api/font/ShapedGlyph.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/canvas/DepthOrder.h>
#include <api/render/realtime/canvas/World.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "CurvePages.h"

#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::paint {

namespace {

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
    // a pixel of the canvas, in the paragraph's pixels, from how much the transform scales area
    const float scale = std::sqrt(std::abs(glm::determinant(glm::mat2(transform))));
    if (canvas == nullptr || scale <= 0.0F || !std::isfinite(scale) || !place(paragraph)) {
        return;
    }
    vertices_.clear();
    indices_.clear();
    for (const Placed& placed : placed_) {
        const std::array<glm::vec2, 4> em = emCorners(placed.stored, 1.0F / (placed.size * scale));
        const std::array<glm::vec2, 4> at = corners(placed, em);
        const auto first = static_cast<std::uint32_t>(vertices_.size());
        for (std::size_t corner = 0; corner < at.size(); corner++) {
            const glm::vec3 moved = transform * glm::vec3(at[corner], 1.0F);
            vertices_.push_back({ .position = glm::vec2(moved), .uv = em[corner], .colour = placed.colour, .glyph = placed.stored.bands });
        }
        for (const std::uint32_t index : TRIANGLES) {
            indices_.push_back(first + index);
        }
    }
    if (!vertices_.empty()) {
        canvas->curves(vertices_, indices_, pages_.curves(), pages_.bands());
    }
}

/**
 **/
void CurveText::draw(render::realtime::canvas::World* canvas, const font::Paragraph& paragraph, const glm::mat4& transform) {
    if (canvas == nullptr || !place(paragraph)) {
        return;
    }
    for (const Placed& placed : placed_) {
        const render::realtime::canvas::World::Glyph glyph = worldGlyph(placed);
        canvas->glyph(inWorld(placed, glyph.em, transform), glyph, placed.colour);
    }
}

/**
 **/
void CurveText::draw(render::realtime::canvas::DepthOrder* order, float key, const font::Paragraph& paragraph, const glm::mat4& transform) {
    if (order == nullptr || !place(paragraph)) {
        return;
    }
    for (const Placed& placed : placed_) {
        const render::realtime::canvas::World::Glyph glyph = worldGlyph(placed);
        order->glyph(key, inWorld(placed, glyph.em, transform), glyph, placed.colour);
    }
}

/**
 **/
const font::CurveStore& CurveText::store() const noexcept {
    return store_;
}

/**
 **/
bool CurveText::place(const font::Paragraph& paragraph) {
    placed_.clear();
    for (const font::Line& line : paragraph.lines()) {
        for (const font::LineRun& run : line.runs) {
            glm::vec2 pen(run.x, line.baseline);
            for (const font::ShapedGlyph& glyph : run.glyphs) {
                const font::StoredCurves& stored = store_.glyph(*run.face, glyph.index);
                if (!stored.empty && run.size > 0.0F) {
                    const glm::vec4& colour = paragraph.styleAt(run.ellipsis ? run.start : glyph.cluster).colour;
                    placed_.push_back(Placed{ .stored = stored, .origin = pen + glyph.offset, .size = run.size, .colour = colour });
                }
                pen.x += glyph.advance;
            }
        }
    }
    // the curves a glyph was just read into have to be on the device before a quad names them
    pages_.sync(&store_);
    return !placed_.empty() && pages_.curves().valid() && pages_.bands().valid();
}

/**
 **/
std::array<glm::vec2, 4> CurveText::emCorners(const font::StoredCurves& stored, float margin) {
    const glm::vec2 low = stored.min - glm::vec2(margin);
    const glm::vec2 high = stored.max + glm::vec2(margin);
    return { glm::vec2(low.x, high.y), glm::vec2(high.x, high.y), glm::vec2(high.x, low.y), glm::vec2(low.x, low.y) };
}

/**
 **/
std::array<glm::vec2, 4> CurveText::corners(const Placed& placed, const std::array<glm::vec2, 4>& em) {
    std::array<glm::vec2, 4> at{};
    for (std::size_t corner = 0; corner < em.size(); corner++) {
        at[corner] = placed.origin + (glm::vec2(em[corner].x, -em[corner].y) * placed.size);
    }
    return at;
}

/**
 **/
render::realtime::canvas::World::Glyph CurveText::worldGlyph(const Placed& placed) const {
    return render::realtime::canvas::World::Glyph{ .em = emCorners(placed.stored, WORLD_MARGIN), .bands = placed.stored.bands,
        .curveTexture = pages_.curves(), .bandTexture = pages_.bands() };
}

/**
 **/
render::realtime::canvas::World::Corners CurveText::inWorld(const Placed& placed, const std::array<glm::vec2, 4>& em, const glm::mat4& transform) {
    const std::array<glm::vec2, 4> at = corners(placed, em);
    render::realtime::canvas::World::Corners world{};
    for (std::size_t corner = 0; corner < at.size(); corner++) {
        world[corner] = glm::vec3(transform * glm::vec4(at[corner], 0.0F, 1.0F));
    }
    return world;
}

};  // namespace v3d::ui::paint
