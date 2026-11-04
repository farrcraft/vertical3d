/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "AreaFrame.h"

#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/paint/Painter.h>

#include <algorithm>
#include <cstddef>
#include <numbers>
#include <optional>
#include <span>

#include "AreaLayout.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::view {

namespace {

/** how many segments trace a quarter turn of a corner **/
constexpr unsigned int CORNER_SIDES = 6;

/** how far past the rounding a corner's cover reaches, which has to pass the corner itself **/
constexpr float CORNER_REACH = 1.5F;

/** how wide the fade is that smooths a corner's rounding, in pixels **/
constexpr float SMOOTHING = 1.0F;

/** the angles a corner's quarter turn starts at, from the side it starts on, turning clockwise **/
constexpr float QUARTER = std::numbers::pi_v<float> / 2.0F;
constexpr float DOWN = QUARTER;
constexpr float LEFT = 2.0F * QUARTER;
constexpr float UP = 3.0F * QUARTER;
constexpr float RIGHT = 0.0F;

/**
 * Cover one corner of an area outside its rounding: a band from the rounding outwards, cut to
 * the square between the corner and the rounding's centre.
 *
 * @param centre the centre the corner turns about
 * @param corner the corner itself
 * @param start the angle the quarter turn starts at
 **/
void cover(v3d::render::realtime::canvas::Canvas* canvas, const glm::vec2& centre, const glm::vec2& corner, float start,
    float radius, const glm::vec4& colour) {
    // the cover fades in across a pixel centred on the rounding, so the curve reads as smooth
    const float half = SMOOTHING * 0.5F;
    const glm::vec4 clear(colour.r, colour.g, colour.b, 0.0F);
    canvas->clip(glm::min(centre, corner), glm::max(centre, corner));
    canvas->ring({ .centre = centre, .outer = radius + half, .inner = radius - half, .sides = CORNER_SIDES,
        .start = start, .sweep = QUARTER }, { .inner = clear, .outer = colour });
    canvas->ring({ .centre = centre, .outer = std::max(radius * CORNER_REACH, radius + SMOOTHING), .inner = radius + half, .sides = CORNER_SIDES,
        .start = start, .sweep = QUARTER }, colour);
    canvas->unclip();
}

};  // namespace

/**
 **/
void drawFrame(v3d::render::realtime::canvas::Canvas* canvas, const AreaLayout& layout, const FrameLook& look,
    std::optional<std::size_t> active, const std::optional<AreaLayout::Divider>& lit, const std::optional<glm::vec4>& landing,
    std::span<const Band> bands) {
    for (const glm::vec4& gap : layout.gaps()) {
        if (gap.z > 0.0F && gap.w > 0.0F) {
            canvas->rect(glm::vec2(gap.x, gap.y), glm::vec2(gap.x + gap.z, gap.y + gap.w), look.gap);
        }
    }
    for (const Band& band : bands) {
        const glm::vec4& box = band.box;
        if (box.z > 0.0F && box.w > 0.0F) {
            canvas->rect(glm::vec2(box.x, box.y), glm::vec2(box.x + box.z, box.y + box.w), band.colour);
        }
    }

    for (std::size_t index = 0; index < layout.areas().size(); index++) {
        const glm::vec4& region = layout.areas()[index].region;
        if (region.z <= 0.0F || region.w <= 0.0F) {
            continue;
        }
        const glm::vec2 min(region.x, region.y);
        const glm::vec2 max(region.x + region.z, region.y + region.w);
        const float radius = std::min({ look.radius, region.z / 2.0F, region.w / 2.0F });
        if (radius > 0.0F) {
            cover(canvas, glm::vec2(min.x + radius, min.y + radius), min, LEFT, radius, look.gap);
            cover(canvas, glm::vec2(max.x - radius, min.y + radius), glm::vec2(max.x, min.y), UP, radius, look.gap);
            cover(canvas, glm::vec2(max.x - radius, max.y - radius), max, RIGHT, radius, look.gap);
            cover(canvas, glm::vec2(min.x + radius, max.y - radius), glm::vec2(min.x, max.y), DOWN, radius, look.gap);
        }
        v3d::ui::paint::strokeBox(canvas, min, max, radius, look.line, active == index ? look.accent : look.hairline);
    }

    if (lit) {
        const glm::vec4& gap = lit->region;
        canvas->rect(glm::vec2(gap.x, gap.y), glm::vec2(gap.x + gap.z, gap.y + gap.w), look.accent);
    }
    if (landing) {
        const glm::vec4& region = *landing;
        canvas->rect(glm::vec2(region.x, region.y), glm::vec2(region.x + region.z, region.y + region.w), look.landing);
    }
}

};  // namespace v3d::editor::view
