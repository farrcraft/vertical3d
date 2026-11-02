/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/canvas/Canvas.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::font {
class GlyphStore;
struct LineRun;
struct TextEffect;
};  // namespace v3d::font

namespace v3d::ui::paint {
class GlyphPages;
};  // namespace v3d::ui::paint

namespace v3d::ui::paint::internal {

/**
 * The quad an effect is drawn over, from its glyph's distance field.
 **/
struct EffectQuad final {
    glm::vec2 min{ 0.0F };   /**< its top left from the glyph's pen, in pixels at the text's size with y downwards **/
    glm::vec2 max{ 0.0F };   /**< and its bottom right **/
    glm::vec2 uv0{ 0.0F };   /**< the field's top left in its page, as a fraction of the page **/
    glm::vec2 uv1{ 0.0F };   /**< and its bottom right **/
    std::size_t page = 0;
    std::uint32_t ramp = 0;  /**< where the effect is whole and where it is gone, packed for the shaders **/
};

/**
 * An effect laid out in a paragraph's space: its quad moved to its glyph's pen, its place among
 * its glyph's effects, and its colour.
 **/
struct PlacedEffect final {
    EffectQuad quad;
    std::size_t layer = 0;
    glm::vec4 colour{ 1.0F };
};

/**
 * Find a glyph's field in a store, made the first time it is asked for with a spread wide
 * enough for the effect, and the quad the effect is drawn over. The field is made from the
 * glyph in its run's face, at the run's coordinates. The quad is the field scaled to the run's
 * size and moved by the effect's offset.
 *
 * @return the quad, or nothing for a glyph with no outline or a field the store cannot hold
 **/
std::optional<EffectQuad> effectQuad(font::GlyphStore* store, const font::LineRun& run, unsigned int index, const font::TextEffect& effect);

/**
 * Lay out each of a glyph's effects at its pen, appending them in the order given.
 *
 * @param pen the glyph's pen in the paragraph's space, in pixels with y downwards
 **/
void placeEffects(std::vector<PlacedEffect>* placed, font::GlyphStore* store, const font::LineRun& run, unsigned int index,
    const std::vector<font::TextEffect>& effects, const glm::vec2& pen);

/**
 * Put laid out effects in the order they are drawn: every glyph's first effect, then every
 * glyph's second, keeping the order of the glyphs within each.
 **/
void layerEffects(std::vector<PlacedEffect>* placed);

/**
 * Append laid out effects to a canvas as distance field text, through an affine transform, one
 * call for each run of quads on one page. Each is corrected as text is, with no contrast.
 *
 * @param vertices and indices are scratch space, kept by the caller so a frame does not
 *        allocate, and left empty
 **/
void drawEffects(render::realtime::canvas::Canvas* canvas, const std::vector<PlacedEffect>& placed, const GlyphPages& pages, const glm::mat3& transform,
    std::vector<render::realtime::canvas::Canvas::Vertex>* vertices, std::vector<std::uint32_t>* indices);

};  // namespace v3d::ui::paint::internal
