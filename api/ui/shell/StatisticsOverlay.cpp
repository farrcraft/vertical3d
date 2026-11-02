/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "StatisticsOverlay.h"

#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Painter.h>
#include <api/ui/paint/TextRenderer.h>
#include <api/ui/paint/Type.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::shell {

/**
 **/
const float StatisticsOverlay::defaultSize = 16.0F;

/**
 **/
StatisticsOverlay::StatisticsOverlay(const boost::shared_ptr<paint::TextRenderer>& text, float size) :
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
void StatisticsOverlay::size(float size) noexcept {
    size_ = size;
}

/**
 **/
float StatisticsOverlay::size() const noexcept {
    return size_;
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
std::string StatisticsOverlay::line(const Sample::Span& span) {
    return span.name + " " + milliseconds(span.nanoseconds);
}

/**
 **/
void StatisticsOverlay::device(const std::string& name) {
    device_ = name;
}

/**
 **/
const std::string& StatisticsOverlay::device() const noexcept {
    return device_;
}

/**
 **/
std::vector<std::string> StatisticsOverlay::content(const Sample& sample) const {
    std::vector<std::string> content;
    if (!device_.empty()) {
        content.push_back(device_);
    }
    const std::array<std::string, rows> frame = lines(sample);
    content.insert(content.end(), frame.begin(), frame.end());
    for (const Sample::Span& span : sample.spans) {
        content.push_back(line(span));
    }
    return content;
}

/**
 **/
void StatisticsOverlay::draw(v3d::render::realtime::canvas::Canvas* canvas, const Sample& sample, const paint::Dressing& dress) {
    if (!visible_ || canvas == nullptr || !text_ || !text_->loaded()) {
        return;
    }

    const std::vector<std::string> content = this->content(sample);
    // its numbers change every frame, so their figures are all one width
    const paint::Type figures = paint::Type(text_, size_).tabular();

    float widest = 0.0F;
    for (const std::string& line : content) {
        widest = std::max(widest, figures.width(line));
    }

    const float lineHeight = size_ * lineSpacing;

    // the box goes on before the glyphs do: the canvas is drawn in the order it is filled,
    // so a background added after the text it backs would cover it
    glm::vec4 plate = dress.panel;
    plate.a *= std::clamp(dress.plateOpacity, 0.0F, 1.0F);
    paint::fillBox(canvas, glm::vec2(margin, margin),
        glm::vec2(margin + widest + (padding * 2.0F), margin + (lineHeight * static_cast<float>(content.size())) + (padding * 2.0F)),
        dress.radius, plate);

    // the pen is the baseline of the line, which sits one font size below the top of it
    float pen = margin + padding + size_;
    for (const std::string& line : content) {
        figures.draw(canvas, line, glm::vec2(margin + padding, pen), dress.text);
        pen += lineHeight;
    }
}

std::string StatisticsOverlay::milliseconds(std::uint64_t nanoseconds) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << (static_cast<double>(nanoseconds) / nanosecondsPerMillisecond) << " ms";
    return out.str();
}

};  // namespace v3d::ui::shell
