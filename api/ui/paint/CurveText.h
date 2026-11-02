/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/CurveStore.h>
#include <api/font/GlyphStore.h>
#include <api/font/Paragraph.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/canvas/DepthOrder.h>
#include <api/render/realtime/canvas/World.h>
#include <api/ui/paint/internal/EffectQuad.h>

#include <array>
#include <cstdint>
#include <vector>

#include "CurvePages.h"
#include "GlyphPages.h"

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::paint {

/**
 * Text drawn from its glyphs' curves, sharp at any size and under any transform: scaled,
 * turned, animated, or standing in the world.
 *
 * Each glyph is a quad over its outline, a little wider on every side so its edge is not cut,
 * whose corners carry their place in the em square. The quad shaders find each pixel's coverage
 * from the curves that cross it. The curves come from a font::CurveStore, read the first time a
 * glyph is drawn at any size, and reach the device through CurvePages.
 *
 * A style's effects are drawn beneath its text from each glyph's distance field, made the
 * first time it is drawn into a font::GlyphStore whose pages reach the device through
 * GlyphPages. Every glyph's effects are drawn before any glyph, and each of a glyph's effects
 * beneath the next. beginFrame() is called once a frame, so the store can make room.
 **/
class CurveText final {
 public:
    /**
     * How much wider than its outline a glyph standing in the world is on every side, in em
     * units. A screen pixel there is not known, so this is enough for text drawn larger than
     * sixteen pixels to the em.
     **/
    static constexpr float WORLD_MARGIN = 1.0F / 16.0F;

    /**
     * @param pages what puts the curves on the device
     * @param fields what puts the effects' distance fields on the device
     **/
    CurveText(CurvePages pages, GlyphPages fields);

    /**
     * Start a frame. A field drawn in the frame before may now make room for another.
     **/
    void beginFrame();

    /**
     * Draw a paragraph's glyphs onto a canvas, each in the colour of its style, with its style's
     * effects beneath it. A glyph's margin is a pixel of the canvas.
     *
     * @param transform from the paragraph's space, in pixels with y downwards, to the canvas's,
     *        as an affine matrix whose last row is 0, 0, 1
     **/
    void draw(render::realtime::canvas::Canvas* canvas, const font::Paragraph& paragraph, const glm::mat3& transform);

    /**
     * Stand a paragraph's glyphs in the world, in the order they are laid out, with their
     * effects before them.
     *
     * @param transform from the paragraph's space, in pixels with y downwards on the plane z = 0,
     *        to the world
     **/
    void draw(render::realtime::canvas::World* canvas, const font::Paragraph& paragraph, const glm::mat4& transform);

    /**
     * Stand a paragraph's glyphs in the world among other quads sorted by depth, every glyph and
     * effect at one key. Each effect's layer is its place among its glyph's effects.
     *
     * @param key how far away the paragraph is, in whatever measure the order sorts by
     **/
    void draw(render::realtime::canvas::DepthOrder* order, float key, const font::Paragraph& paragraph, const glm::mat4& transform);

    /**
     * @return the store the curves are kept in
     **/
    const font::CurveStore& store() const noexcept;

    /**
     * @return the store the effects' distance fields are kept in
     **/
    const font::GlyphStore& fields() const noexcept;

 private:
    /**
     * A glyph laid out in the paragraph's space: its curves, its pen, its size in pixels, and
     * its colour.
     **/
    struct Placed {
        font::StoredCurves stored;
        glm::vec2 origin{ 0.0F };
        float size = 0.0F;
        glm::vec4 colour{ 1.0F };
        float contrast = 0.0F;
    };

    /**
     * Lay a paragraph's glyphs out into placed_ and their effects into effects_, read the curves
     * and make the fields of any not yet stored, and bring the device's copies in step.
     *
     * @return whether the curves are on the device to draw from
     **/
    bool place(const font::Paragraph& paragraph);

    /**
     * @return a glyph's em corners, wider than its outline by a margin in em units: top left,
     *         top right, bottom right, bottom left, with y upwards
     **/
    static std::array<glm::vec2, 4> emCorners(const font::StoredCurves& stored, float margin);

    /**
     * @return the corners in the paragraph's space, with y downwards
     **/
    static std::array<glm::vec2, 4> corners(const Placed& placed, const std::array<glm::vec2, 4>& em);

    /**
     * @return what a world canvas draws a glyph with
     **/
    render::realtime::canvas::World::Glyph worldGlyph(const Placed& placed) const;

    /**
     * @return a glyph's corners standing in the world
     **/
    static render::realtime::canvas::World::Corners inWorld(const Placed& placed, const std::array<glm::vec2, 4>& em, const glm::mat4& transform);

    /**
     * @return an effect's corners standing in the world: top left, top right, bottom right,
     *         bottom left in the paragraph's space
     **/
    static render::realtime::canvas::World::Corners inWorld(const internal::PlacedEffect& effect, const glm::mat4& transform);

    font::CurveStore store_;
    CurvePages pages_;
    font::GlyphStore fields_;
    GlyphPages fieldPages_;
    // what a draw lays out and hands the canvas, kept between draws so a frame does not allocate
    std::vector<Placed> placed_;
    std::vector<internal::PlacedEffect> effects_;
    std::vector<render::realtime::canvas::Canvas::Vertex> vertices_;
    std::vector<std::uint32_t> indices_;
};

};  // namespace v3d::ui::paint
