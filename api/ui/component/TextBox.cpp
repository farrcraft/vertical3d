/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextBox.h"

#include <cstddef>
#include <string>

namespace v3d::ui::component {

namespace {

/**
 * Whether a byte is a utf-8 continuation - the ones a character boundary is never on.
 **/
bool continuation(char byte) noexcept {
    return (static_cast<unsigned char>(byte) & 0xC0U) == 0x80U;
}

};  // namespace

TextBox::TextBox() :
    Component(component::Type::TextBox),
    caret_(0),
    limit_(0) {
    // a box exists to be typed into, so it asks for the press and the focus that a panel
    // laid over a scene must not take - ADR-0034 and ADR-0040
    pickable(true);
    focusable(true);
}

void TextBox::text(const std::string& value) {
    text_ = value;
    caret_ = text_.size();
}

std::string_view TextBox::text() const noexcept {
    return text_;
}

std::size_t TextBox::boundary(std::size_t offset) const noexcept {
    std::size_t at = offset > text_.size() ? text_.size() : offset;
    while (at > 0 && at < text_.size() && continuation(text_[at])) {
        at--;
    }
    return at;
}

void TextBox::caret(std::size_t offset) {
    caret_ = boundary(offset);
}

std::size_t TextBox::caret() const noexcept {
    return caret_;
}

void TextBox::limit(std::size_t bytes) noexcept {
    limit_ = bytes;
}

std::size_t TextBox::limit() const noexcept {
    return limit_;
}

void TextBox::placeholder(const std::string& value) {
    placeholder_ = value;
}

std::string_view TextBox::placeholder() const noexcept {
    return placeholder_;
}

bool TextBox::insert(std::string_view value) {
    if (value.empty()) {
        return false;
    }
    // refused whole rather than truncated: half a pasted path is worse than none of it,
    // and truncating utf-8 by bytes can cut a character in two
    if (limit_ > 0 && text_.size() + value.size() > limit_) {
        return false;
    }
    text_.insert(caret_, value);
    caret_ += value.size();
    return true;
}

bool TextBox::backspace() {
    if (caret_ == 0) {
        return false;
    }
    std::size_t from = caret_ - 1;
    while (from > 0 && continuation(text_[from])) {
        from--;
    }
    text_.erase(from, caret_ - from);
    caret_ = from;
    return true;
}

bool TextBox::erase() {
    if (caret_ >= text_.size()) {
        return false;
    }
    std::size_t to = caret_ + 1;
    while (to < text_.size() && continuation(text_[to])) {
        to++;
    }
    text_.erase(caret_, to - caret_);
    return true;
}

bool TextBox::left() {
    if (caret_ == 0) {
        return false;
    }
    caret_--;
    while (caret_ > 0 && continuation(text_[caret_])) {
        caret_--;
    }
    return true;
}

bool TextBox::right() {
    if (caret_ >= text_.size()) {
        return false;
    }
    caret_++;
    while (caret_ < text_.size() && continuation(text_[caret_])) {
        caret_++;
    }
    return true;
}

bool TextBox::home() {
    if (caret_ == 0) {
        return false;
    }
    caret_ = 0;
    return true;
}

bool TextBox::end() {
    if (caret_ >= text_.size()) {
        return false;
    }
    caret_ = text_.size();
    return true;
}

void TextBox::event(const v3d::event::Event& destination) {
    event_ = destination;
}

v3d::event::Event TextBox::event() const {
    return event_;
}

};  // namespace v3d::ui::component
