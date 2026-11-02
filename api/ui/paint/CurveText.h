/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/CurveStore.h>
#include <api/font/Paragraph.h>
#include <api/render/realtime/canvas/Canvas.h>

#include <cstdint>
#include <vector>

#include "CurvePages.h"

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::paint {

/**
 * Text drawn from its glyphs' curves, sharp at any size and under any transform: scaled,
 * turned, or animated.
 *
 * Each glyph is a quad over its outline, a pixel wider on every side at the transform's scale so
 * its edge is not cut, whose corners carry their place in the em square. The quad shader finds
 * each pixel's coverage from the curves that cross it. The curves come from a font::CurveStore,
 * read the first time a glyph is drawn at any size, and reach the device through CurvePages.
 **/
class CurveText final {
 public:
    explicit CurveText(CurvePages pages);

    /**
     * Draw a paragraph's glyphs onto a canvas, each in the colour of its style.
     *
     * @param transform from the paragraph's space, in pixels with y downwards, to the canvas's,
     *        as an affine matrix whose last row is 0, 0, 1
     **/
    void draw(render::realtime::canvas::Canvas* canvas, const font::Paragraph& paragraph, const glm::mat3& transform);

    /**
     * @return the store the curves are kept in
     **/
    const font::CurveStore& store() const noexcept;

 private:
    /**
     * Where a glyph goes: its pen, its size in pixels, the margin around its outline in em units,
     * and the transform into the canvas.
     **/
    struct Placing {
        glm::vec2 origin{ 0.0F };
        float size = 0.0F;
        float margin = 0.0F;
        glm::mat3 transform{ 1.0F };
    };

    /**
     * Add a glyph's quad over its outline, wider by the margin on every side.
     **/
    void quad(const font::StoredCurves& stored, const Placing& placing, const glm::vec4& colour);

    font::CurveStore store_;
    CurvePages pages_;
    // what draw() hands the canvas, kept between draws so a frame of text does not allocate
    std::vector<render::realtime::canvas::Canvas::Vertex> vertices_;
    std::vector<std::uint32_t> indices_;
};

};  // namespace v3d::ui::paint
