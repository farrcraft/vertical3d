/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Type.h"

#include <api/font/text/Feature.h>
#include <api/font/text/Paragraph.h>
#include <api/font/text/Style.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Text.h"
#include "TextRenderer.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::paint {

/**
 **/
Type::Type(const boost::shared_ptr<TextRenderer>& text, float size, Weight weight) :
    text_(text),
    size_(size),
    weight_(weight) {
}

/**
 **/
bool Type::set() const noexcept {
    return static_cast<bool>(text_);
}

/**
 **/
float Type::width(std::string_view text) const {
    return text_ ? text_->width(text, style()) : 0.0F;
}

/**
 **/
std::vector<std::string> Type::rows(std::string_view text, float width) const {
    return text_ ? text_->rows(text, width, style()) : std::vector<std::string>();
}

/**
 **/
std::optional<font::text::Paragraph> Type::paragraph(const std::string& text) const {
    if (!text_ || !text_->loaded()) {
        return std::nullopt;
    }
    return font::text::Paragraph(text_->shaping(), text, style());
}

/**
 **/
void Type::draw(v3d::render::realtime::canvas::Canvas* canvas, std::string_view text, const glm::vec2& pen, const glm::vec4& colour) const {
    if (text_) {
        font::text::Style styled = style();
        styled.colour = colour;
        text_->draw(canvas, text, pen, styled);
    }
}

/**
 **/
Type Type::in(Weight weight) const {
    Type changed = *this;
    changed.weight_ = weight;
    return changed;
}

/**
 **/
Type Type::scaled(float factor) const {
    float size = size_;
    if (size <= 0.0F && text_) {
        size = text_->size();
    }
    Type changed = *this;
    changed.size_ = size * factor;
    return changed;
}

/**
 **/
Type Type::tabular() const {
    Type changed = *this;
    changed.tabular_ = true;
    return changed;
}

/**
 **/
font::text::Style Type::style() const {
    if (!text_) {
        return font::text::Style();
    }
    font::text::Style styled = text_->style(size_, weight_);
    if (tabular_) {
        styled.features.push_back(font::text::Feature{ .tag = "tnum" });
    }
    return styled;
}

/**
 **/
const boost::shared_ptr<TextRenderer>& Type::renderer() const noexcept {
    return text_;
}

/**
 **/
float Type::size() const noexcept {
    return size_;
}

/**
 **/
Weight Type::weight() const noexcept {
    return weight_;
}

};  // namespace v3d::ui::paint
