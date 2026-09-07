/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "StatisticsOverlay.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

/**
 * The gap from the corner of the canvas to the box, and from the box to the text inside it.
 **/
const float margin = 8.0f;
const float padding = 6.0f;

/**
 * A line's height as a multiple of the size the font was rasterized at. Nothing scales a
 * glyph, so this is the only thing that decides how far apart the lines sit.
 **/
const float lineSpacing = 1.3f;

/**
 * Translucent so that what is being measured stays visible under the numbers.
 **/
constexpr glm::vec4 background(0.0f, 0.0f, 0.0f, 0.6f);
constexpr glm::vec4 foreground(0.9f, 0.9f, 0.9f, 1.0f);

const double nanosecondsPerMillisecond = 1000000.0;
const double nanosecondsPerSecond = 1000000000.0;

/**
 * A duration in nanoseconds, as milliseconds to one decimal place.
 **/
std::string milliseconds(std::uint64_t nanoseconds) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << (static_cast<double>(nanoseconds) / nanosecondsPerMillisecond) << " ms";
    return out.str();
}

};  // namespace

namespace v3d::ui {

/**
 **/
const float StatisticsOverlay::defaultSize = 16.0f;

/**
 **/
StatisticsOverlay::StatisticsOverlay(const boost::shared_ptr<TextRenderer>& text, float size) :
    text_(text),
    size_(size),
    visible_(false) {
}

/**
 **/
void StatisticsOverlay::toggle() noexcept {
    visible_ = !visible_;
}

/**
 **/
bool StatisticsOverlay::visible() const noexcept {
    return visible_;
}

/**
 **/
void StatisticsOverlay::visible(bool visible) noexcept {
    visible_ = visible;
}

/**
 **/
std::array<std::string, StatisticsOverlay::rows> StatisticsOverlay::lines(const Sample& sample) {
    std::ostringstream mean;
    mean << milliseconds(sample.mean);
    // a rate needs a duration to divide into, and there is none until the first frame has
    // been recorded, so the frame time is shown on its own until there is one
    if (sample.mean > 0) {
        mean << "  " << std::llround(nanosecondsPerSecond / static_cast<double>(sample.mean)) << " fps";
    }

    std::ostringstream steps;
    steps << "steps " << sample.steps;

    return { mean.str(), "last " + milliseconds(sample.last), steps.str() };
}

/**
 **/
void StatisticsOverlay::draw(v3d::render::realtime::Canvas* canvas, const Sample& sample) {
    if (!visible_ || !canvas || !text_ || !text_->loaded()) {
        return;
    }

    const std::array<std::string, rows> content = lines(sample);

    float widest = 0.0f;
    for (const std::string& line : content) {
        widest = std::max(widest, text_->width(line, size_));
    }

    const float lineHeight = size_ * lineSpacing;

    // the box goes on before the glyphs do: the canvas is drawn in the order it is filled,
    // so a background added after the text it backs would cover it
    canvas->rect(glm::vec2(margin, margin),
        glm::vec2(margin + widest + padding * 2.0f, margin + lineHeight * static_cast<float>(rows) + padding * 2.0f),
        background);

    // the pen is the baseline of the line, which sits one font size below the top of it
    float pen = margin + padding + size_;
    for (const std::string& line : content) {
        text_->draw(canvas, line, glm::vec2(margin + padding, pen), foreground, size_);
        pen += lineHeight;
    }
}

};  // namespace v3d::ui
