/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Type.h"

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
    return text_ ? text_->width(text, size_, weight_) : 0.0F;
}

/**
 **/
std::vector<std::string> Type::rows(std::string_view text, float width) const {
    return text_ ? text_->rows(text, width, text_->style(size_, weight_)) : std::vector<std::string>();
}

/**
 **/
void Type::draw(v3d::render::realtime::canvas::Canvas* canvas, std::string_view text, const glm::vec2& pen, const glm::vec4& colour) const {
    if (text_) {
        text_->draw(canvas, text, pen, colour, size_, weight_);
    }
}

/**
 **/
Type Type::in(Weight weight) const {
    return Type(text_, size_, weight);
}

/**
 **/
Type Type::scaled(float factor) const {
    float size = size_;
    if (size <= 0.0F && text_) {
        size = text_->size();
    }
    return Type(text_, size * factor, weight_);
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
