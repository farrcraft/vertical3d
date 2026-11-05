/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/glyph/CurveStore.h>
#include <api/font/glyph/Store.h>
#include <api/font/text/Paragraph.h>
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
 * from the curves that cross it. The curves come from a font::glyph::CurveStore, read the first
 * time a glyph is drawn at any size, and reach the device through CurvePages.
 *
 * A glyph its face draws in colour is drawn from its layers' curves, through a program the
 * store keeps, in the face's first palette. A layer that takes the text's colour takes it, and
 * the text's alpha fades it. A glyph whose colour is a picture, from a bitmap table, is painted
 * at its size into a store of colour pages. It is drawn as a picture through the same
 * transform, so it softens when scaled up.
 *
 * A style's effects are drawn beneath its text from each glyph's distance field, made the
 * first time it is drawn into a font::glyph::Store whose pages reach the device through
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
     * @param fields what puts the effects' distance fields, and the pictures of colour glyphs,
     *        on the device
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
     * @param canvas what the glyphs are drawn on, or null to draw nothing
     * @param paragraph the laid out text drawn
     * @param transform from the paragraph's space, in pixels with y downwards, to the canvas's,
     *        as an affine matrix whose last row is 0, 0, 1
     **/
    void draw(render::realtime::canvas::Canvas* canvas, const font::text::Paragraph& paragraph, const glm::mat3& transform);

    /**
     * Stand a paragraph's glyphs in the world, in the order they are laid out, with their
     * effects before them.
     *
     * @param canvas what the glyphs are drawn into, or null to draw nothing
     * @param paragraph the laid out text drawn
     * @param transform from the paragraph's space, in pixels with y downwards on the plane z = 0,
     *        to the world
     **/
    void draw(render::realtime::canvas::World* canvas, const font::text::Paragraph& paragraph, const glm::mat4& transform);

    /**
     * Stand a paragraph's glyphs in the world among other quads sorted by depth, every glyph and
     * effect at one key. Each effect's layer is its place among its glyph's effects.
     *
     * @param order what the glyphs are sorted into, or null to draw nothing
     * @param key how far away the paragraph is, in whatever measure the order sorts by
     * @param paragraph the laid out text drawn
     * @param transform from the paragraph's space, in pixels with y downwards on the plane
     *        z = 0, to the world
     **/
    void draw(render::realtime::canvas::DepthOrder* order, float key, const font::text::Paragraph& paragraph, const glm::mat4& transform);

    /**
     * @return the store the curves are kept in
     **/
    const font::glyph::CurveStore& store() const noexcept;

    /**
     * @return the store the effects' distance fields are kept in
     **/
    const font::glyph::Store& fields() const noexcept;

 private:
    /**
     * A glyph laid out in the paragraph's space: its curves, its pen, its size in pixels, and
     * its colour.
     **/
    struct Placed {
        font::glyph::StoredCurves stored;
        glm::vec2 origin{ 0.0F };
        float size = 0.0F;
        glm::vec4 colour{ 1.0F };
        float contrast = 0.0F;
    };

    /**
     * A colour glyph's picture laid out in the paragraph's space: where it is stored, its top
     * left and bottom right, and its colour, white faded by the text's alpha.
     **/
    struct PlacedPicture {
        font::glyph::StoredGlyph glyph;
        glm::vec2 low{ 0.0F };
        glm::vec2 high{ 0.0F };
        glm::vec4 colour{ 1.0F };
    };

    /**
     * Place a glyph its face draws in colour, as a program or as a picture.
     *
     * @return whether it was placed, and false for a glyph whose colour draws nothing
     **/
    bool placeColour(const font::text::LineRun& run, unsigned int index, const font::text::Style& styled, const glm::vec2& origin);

    /**
     * @return a picture's corners in the paragraph's space: top left, top right, bottom right,
     *         bottom left
     **/
    static std::array<glm::vec2, 4> pictureCorners(const PlacedPicture& picture);

    /**
     * @return a picture's corners standing in the world, in its corners' order
     **/
    static render::realtime::canvas::World::Corners inWorld(const PlacedPicture& picture, const glm::mat4& transform);

    /**
     * @return a picture's uv rectangle in its page: top left, then bottom right
     **/
    std::array<glm::vec2, 2> pictureUv(const PlacedPicture& picture) const;

    /**
     * Lay a paragraph's glyphs out into placed_ and their effects into effects_, read the curves
     * and make the fields of any not yet stored, and bring the device's copies in step.
     *
     * @return whether the curves are on the device to draw from
     **/
    bool place(const font::text::Paragraph& paragraph);

    /**
     * @return a glyph's em corners, wider than its outline by a margin in em units: top left,
     *         top right, bottom right, bottom left, with y upwards
     **/
    static std::array<glm::vec2, 4> emCorners(const font::glyph::StoredCurves& stored, float margin);

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

    font::glyph::CurveStore store_;
    CurvePages pages_;
    font::glyph::Store fields_;
    GlyphPages fieldPages_;
    font::glyph::Store pictures_;
    GlyphPages picturePages_;
    // what a draw lays out and hands the canvas, kept between draws so a frame does not allocate
    std::vector<Placed> placed_;
    std::vector<internal::PlacedEffect> effects_;
    std::vector<PlacedPicture> placedPictures_;
    std::vector<render::realtime::canvas::Canvas::Vertex> vertices_;
    std::vector<std::uint32_t> indices_;
};

};  // namespace v3d::ui::paint
