/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CurveText.h"

#include <api/font/glyph/CurveStore.h>
#include <api/font/face/Face.h>
#include <api/font/glyph/Store.h>
#include <api/font/glyph/Paint.h>
#include <api/font/text/Paragraph.h>
#include <api/font/text/ShapedGlyph.h>
#include <api/font/text/Style.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/canvas/DepthOrder.h>
#include <api/render/realtime/canvas/World.h>
#include <api/ui/paint/internal/EffectQuad.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "CurvePages.h"
#include "GlyphPages.h"

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

/**
 * @return the pages a colour glyph's picture is kept in: the default pages, of four bytes a
 *         pixel
 **/
font::glyph::Store::Spec picturePages() {
    font::glyph::Store::Spec spec;
    spec.channels = 4;
    return spec;
}

};  // namespace

/**
 **/
CurveText::CurveText(CurvePages pages, GlyphPages fields) :
    pages_(std::move(pages)),
    fieldPages_(fields),
    pictures_(picturePages()),
    picturePages_(std::move(fields)) {
}

/**
 **/
void CurveText::beginFrame() {
    fields_.beginFrame();
    pictures_.beginFrame();
}

/**
 **/
void CurveText::draw(render::realtime::canvas::Canvas* canvas, const font::text::Paragraph& paragraph, const glm::mat3& transform) {
    // a pixel of the canvas, in the paragraph's pixels, from how much the transform scales area
    const float scale = std::sqrt(std::abs(glm::determinant(glm::mat2(transform))));
    if (canvas == nullptr || scale <= 0.0F || !std::isfinite(scale) || !place(paragraph)) {
        return;
    }
    internal::drawEffects(canvas, effects_, fieldPages_, transform, &vertices_, &indices_);
    for (const Placed& placed : placed_) {
        const std::array<glm::vec2, 4> em = emCorners(placed.stored, 1.0F / (placed.size * scale));
        const std::array<glm::vec2, 4> at = corners(placed, em);
        const auto first = static_cast<std::uint32_t>(vertices_.size());
        for (std::size_t corner = 0; corner < at.size(); corner++) {
            const glm::vec3 moved = transform * glm::vec3(at[corner], 1.0F);
            vertices_.push_back({ .position = glm::vec2(moved), .uv = em[corner], .colour = placed.colour, .glyph = placed.stored.bands, .contrast = placed.contrast });
        }
        for (const std::uint32_t index : TRIANGLES) {
            indices_.push_back(first + index);
        }
    }
    if (!vertices_.empty()) {
        canvas->curves(vertices_, indices_, pages_.curves(), pages_.bands());
        vertices_.clear();
        indices_.clear();
    }
    // a picture is a quad of its colour page, through the same transform
    for (std::size_t index = 0; index < placedPictures_.size(); index++) {
        const PlacedPicture& picture = placedPictures_[index];
        const std::array<glm::vec2, 4> at = pictureCorners(picture);
        const std::array<glm::vec2, 2> uv = pictureUv(picture);
        const std::array<glm::vec2, 4> uvs{ uv[0], glm::vec2(uv[1].x, uv[0].y), uv[1], glm::vec2(uv[0].x, uv[1].y) };
        const auto first = static_cast<std::uint32_t>(vertices_.size());
        for (std::size_t corner = 0; corner < at.size(); corner++) {
            vertices_.push_back({ .position = glm::vec2(transform * glm::vec3(at[corner], 1.0F)), .uv = uvs[corner], .colour = picture.colour });
        }
        for (const std::uint32_t triangle : TRIANGLES) {
            indices_.push_back(first + triangle);
        }
        if (index + 1 == placedPictures_.size() || placedPictures_[index + 1].glyph.page != picture.glyph.page) {
            canvas->glyphs(vertices_, indices_, picturePages_.texture(picture.glyph.page));
            vertices_.clear();
            indices_.clear();
        }
    }
}

/**
 **/
void CurveText::draw(render::realtime::canvas::World* canvas, const font::text::Paragraph& paragraph, const glm::mat4& transform) {
    if (canvas == nullptr || !place(paragraph)) {
        return;
    }
    for (const internal::PlacedEffect& effect : effects_) {
        canvas->field(inWorld(effect, transform), effect.quad.uv0, effect.quad.uv1, effect.colour, fieldPages_.texture(effect.quad.page), effect.quad.ramp);
    }
    for (const Placed& placed : placed_) {
        const render::realtime::canvas::World::Glyph glyph = worldGlyph(placed);
        canvas->glyph(inWorld(placed, glyph.em, transform), glyph, placed.colour);
    }
    for (const PlacedPicture& picture : placedPictures_) {
        const std::array<glm::vec2, 2> uv = pictureUv(picture);
        canvas->quad(inWorld(picture, transform), uv[0], uv[1], picture.colour, picturePages_.texture(picture.glyph.page));
    }
}

/**
 **/
void CurveText::draw(render::realtime::canvas::DepthOrder* order, float key, const font::text::Paragraph& paragraph, const glm::mat4& transform) {
    if (order == nullptr || !place(paragraph)) {
        return;
    }
    for (const internal::PlacedEffect& effect : effects_) {
        order->field(key, static_cast<std::uint32_t>(effect.layer), inWorld(effect, transform), effect.quad.uv0, effect.quad.uv1, effect.colour,
            fieldPages_.texture(effect.quad.page), effect.quad.ramp);
    }
    for (const Placed& placed : placed_) {
        const render::realtime::canvas::World::Glyph glyph = worldGlyph(placed);
        order->glyph(key, inWorld(placed, glyph.em, transform), glyph, placed.colour);
    }
    for (const PlacedPicture& picture : placedPictures_) {
        const std::array<glm::vec2, 2> uv = pictureUv(picture);
        order->quad(key, inWorld(picture, transform), uv[0], uv[1], picture.colour, picturePages_.texture(picture.glyph.page));
    }
}

/**
 **/
const font::glyph::CurveStore& CurveText::store() const noexcept {
    return store_;
}

/**
 **/
const font::glyph::Store& CurveText::fields() const noexcept {
    return fields_;
}

/**
 **/
bool CurveText::place(const font::text::Paragraph& paragraph) {
    placed_.clear();
    effects_.clear();
    placedPictures_.clear();
    for (const font::text::Line& line : paragraph.lines()) {
        for (const font::text::LineRun& run : line.runs) {
            glm::vec2 pen(run.x, line.baseline);
            for (const font::text::ShapedGlyph& glyph : run.glyphs) {
                const font::text::Style& styled = paragraph.styleAt(run.ellipsis ? run.start : glyph.cluster);
                const glm::vec2 origin = pen + glyph.offset;
                pen.x += glyph.advance;
                if (!(run.size > 0.0F)) {
                    continue;
                }
                internal::placeEffects(&effects_, &fields_, run, glyph.index, styled.effects, origin);
                if (run.face->coloured(glyph.index) && placeColour(run, glyph.index, styled, origin)) {
                    continue;
                }
                const font::glyph::StoredCurves& stored = store_.glyph(*run.face, glyph.index, run.coordinates);
                if (!stored.empty) {
                    placed_.push_back(Placed{ .stored = stored, .origin = origin, .size = run.size, .colour = styled.colour, .contrast = styled.contrast });
                }
            }
        }
    }
    internal::layerEffects(&effects_);
    // a glyph's new curves, fields and pictures have to be on the device before a quad names them
    pages_.sync(&store_);
    fieldPages_.sync(&fields_);
    picturePages_.sync(&pictures_);
    const bool curves = !placed_.empty() && pages_.curves().valid() && pages_.bands().valid();
    return curves || !placedPictures_.empty();
}

/**
 **/
bool CurveText::placeColour(const font::text::LineRun& run, unsigned int index, const font::text::Style& styled, const glm::vec2& origin) {
    const font::glyph::StoredColour& stored = store_.colour(*run.face, index, 0, run.coordinates);
    if (stored.pictures) {
        const font::glyph::Key key{ .face = run.face.get(), .index = index, .size = run.size, .mode = font::glyph::Mode::Colour,
            .foreground = font::glyph::packColour(glm::vec4(glm::vec3(styled.colour), 1.0F)), .coordinates = run.coordinates };
        const std::optional<font::glyph::StoredGlyph> painted = pictures_.glyph(key, [](const font::glyph::Key& asked) { return font::glyph::paint(asked); });
        if (!painted || painted->size.x == 0 || painted->size.y == 0) {
            return false;
        }
        const glm::vec2 low = origin + glm::vec2(painted->bearing);
        placedPictures_.push_back(PlacedPicture{ .glyph = *painted, .low = low, .high = low + glm::vec2(painted->size),
            .colour = glm::vec4(1.0F, 1.0F, 1.0F, styled.colour.a) });
        return true;
    }
    if (stored.empty) {
        return false;
    }
    // a program draws in its own colours, so it is not text whose coverage is corrected
    const font::glyph::StoredCurves program{ .bands = stored.program | font::glyph::CurveStore::COLOUR, .min = stored.min, .max = stored.max, .empty = false };
    placed_.push_back(Placed{ .stored = program, .origin = origin, .size = run.size, .colour = styled.colour, .contrast = -1.0F });
    return true;
}

/**
 **/
std::array<glm::vec2, 4> CurveText::pictureCorners(const PlacedPicture& picture) {
    return { picture.low, glm::vec2(picture.high.x, picture.low.y), picture.high, glm::vec2(picture.low.x, picture.high.y) };
}

/**
 **/
render::realtime::canvas::World::Corners CurveText::inWorld(const PlacedPicture& picture, const glm::mat4& transform) {
    const std::array<glm::vec2, 4> at = pictureCorners(picture);
    render::realtime::canvas::World::Corners world{};
    for (std::size_t corner = 0; corner < at.size(); corner++) {
        world[corner] = glm::vec3(transform * glm::vec4(at[corner], 0.0F, 1.0F));
    }
    return world;
}

/**
 **/
std::array<glm::vec2, 2> CurveText::pictureUv(const PlacedPicture& picture) const {
    const auto side = static_cast<float>(pictures_.spec().pageSize);
    const glm::vec2 from = glm::vec2(picture.glyph.position) / side;
    return { from, from + (glm::vec2(picture.glyph.size) / side) };
}

/**
 **/
std::array<glm::vec2, 4> CurveText::emCorners(const font::glyph::StoredCurves& stored, float margin) {
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
render::realtime::canvas::World::Corners CurveText::inWorld(const internal::PlacedEffect& effect, const glm::mat4& transform) {
    const glm::vec2& low = effect.quad.min;
    const glm::vec2& high = effect.quad.max;
    const std::array<glm::vec2, 4> at{ low, glm::vec2(high.x, low.y), high, glm::vec2(low.x, high.y) };
    render::realtime::canvas::World::Corners world{};
    for (std::size_t corner = 0; corner < at.size(); corner++) {
        world[corner] = glm::vec3(transform * glm::vec4(at[corner], 0.0F, 1.0F));
    }
    return world;
}

/**
 **/
render::realtime::canvas::World::Glyph CurveText::worldGlyph(const Placed& placed) const {
    return render::realtime::canvas::World::Glyph{ .em = emCorners(placed.stored, WORLD_MARGIN), .bands = placed.stored.bands,
        .curveTexture = pages_.curves(), .bandTexture = pages_.bands(), .contrast = placed.contrast };
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
