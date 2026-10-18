/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextBox.h"

#include <api/event/Event.h>
#include <api/event/Type.h>
#include <api/ui/Component.h>
#include <api/ui/paint/Text.h>

#include "Type.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace v3d::ui::component {

namespace {

/**
 * The top two bits of a utf-8 byte, which say whether it is a continuation, and the value
 * they hold in one.
 **/
constexpr unsigned int CONTINUATION_MASK = 0xC0U;
constexpr unsigned int CONTINUATION_TAG = 0x80U;

/**
 * Whether a byte is a utf-8 continuation - the ones a character boundary is never on.
 **/
bool continuation(char byte) noexcept {
    return (static_cast<unsigned char>(byte) & CONTINUATION_MASK) == CONTINUATION_TAG;
}

};  // namespace

TextBox::TextBox() :
    Component(component::Type::TextBox),
    caret_(0),
    anchor_(0),
    limit_(0),
    pen_(0.0F) {
    // a control is pickable and focusable from the start. A plain component is neither, so a
    // panel laid over a scene lets presses through
    pickable(true);
    focusable(true);
}

void TextBox::text(const std::string& value) {
    text_ = value;
    caret_ = text_.size();
    anchor_ = caret_;
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

std::size_t TextBox::forward(std::size_t offset) const noexcept {
    std::size_t at = offset >= text_.size() ? text_.size() : offset + 1;
    while (at < text_.size() && continuation(text_[at])) {
        at++;
    }
    return at;
}

std::pair<std::size_t, std::size_t> TextBox::run() const noexcept {
    return std::make_pair(std::min(caret_, anchor_), std::max(caret_, anchor_));
}

void TextBox::caret(std::size_t offset, bool extend) {
    caret_ = boundary(offset);
    if (!extend) {
        anchor_ = caret_;
    }
}

std::size_t TextBox::caret() const noexcept {
    return caret_;
}

std::size_t TextBox::anchor() const noexcept {
    return anchor_;
}

void TextBox::select(std::size_t from, std::size_t to) {
    anchor_ = boundary(from);
    caret_ = boundary(to);
}

void TextBox::selectAll() {
    anchor_ = 0;
    caret_ = text_.size();
}

void TextBox::deselect() noexcept {
    anchor_ = caret_;
}

bool TextBox::selected() const noexcept {
    return anchor_ != caret_;
}

std::string_view TextBox::selection() const noexcept {
    const std::pair<std::size_t, std::size_t> span = run();
    return std::string_view(text_).substr(span.first, span.second - span.first);
}

bool TextBox::removeSelection() {
    if (!selected()) {
        return false;
    }
    const std::pair<std::size_t, std::size_t> span = run();
    text_.erase(span.first, span.second - span.first);
    caret_ = span.first;
    anchor_ = caret_;
    return true;
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
    const std::pair<std::size_t, std::size_t> span = run();
    // refused whole, not truncated, so a paste never lands half a path, and utf-8 is never
    // cut inside a character. Measured against what the text would become, so replacing a
    // selection makes room for what replaces it
    if (limit_ > 0 && text_.size() - (span.second - span.first) + value.size() > limit_) {
        return false;
    }
    removeSelection();
    text_.insert(caret_, value);
    caret_ += value.size();
    anchor_ = caret_;
    return true;
}

bool TextBox::backspace() {
    if (removeSelection()) {
        // with a selection, a backspace removes only the selection
        return true;
    }
    if (caret_ == 0) {
        return false;
    }
    std::size_t from = caret_ - 1;
    while (from > 0 && continuation(text_[from])) {
        from--;
    }
    text_.erase(from, caret_ - from);
    caret_ = from;
    anchor_ = caret_;
    return true;
}

bool TextBox::erase() {
    if (removeSelection()) {
        return true;
    }
    if (caret_ >= text_.size()) {
        return false;
    }
    text_.erase(caret_, forward(caret_) - caret_);
    return true;
}

bool TextBox::left(bool extend) {
    if (!extend && selected()) {
        // the near end of the selection, rather than a character past it
        caret_ = run().first;
        anchor_ = caret_;
        return true;
    }
    if (caret_ == 0) {
        return false;
    }
    caret_--;
    while (caret_ > 0 && continuation(text_[caret_])) {
        caret_--;
    }
    if (!extend) {
        anchor_ = caret_;
    }
    return true;
}

bool TextBox::right(bool extend) {
    if (!extend && selected()) {
        caret_ = run().second;
        anchor_ = caret_;
        return true;
    }
    if (caret_ >= text_.size()) {
        return false;
    }
    caret_ = forward(caret_);
    if (!extend) {
        anchor_ = caret_;
    }
    return true;
}

bool TextBox::home(bool extend) {
    const bool moved = caret_ != 0 || (!extend && selected());
    caret_ = 0;
    if (!extend) {
        anchor_ = caret_;
    }
    return moved;
}

bool TextBox::end(bool extend) {
    const bool moved = caret_ != text_.size() || (!extend && selected());
    caret_ = text_.size();
    if (!extend) {
        anchor_ = caret_;
    }
    return moved;
}

std::size_t TextBox::at(const glm::vec2& point, const paint::Measure& measure) const {
    if (!measure || text_.empty()) {
        return 0;
    }
    const float into = point.x - pen_;
    if (into <= 0.0F) {
        return 0;
    }

    // step through the boundaries, and stop at the first character whose middle the point
    // is not past. The left half of a character puts the caret before it and the right
    // half after, so a click between two letters lands between them
    std::size_t found = 0;
    float before = 0.0F;
    std::size_t offset = 0;
    while (offset < text_.size()) {
        const std::size_t after = forward(offset);
        const float width = measure(std::string_view(text_).substr(0, after));
        if (into < (before + width) * 0.5F) {
            break;
        }
        found = after;
        before = width;
        offset = after;
    }
    return found;
}

void TextBox::pen(float x) noexcept {
    pen_ = x;
}

float TextBox::pen() const noexcept {
    return pen_;
}

void TextBox::event(const v3d::event::Event& destination) {
    event_ = destination;
    // marked as a Destination event here, not by the caller, because a listener that accepts
    // only Destination events drops an unmarked one
    event_.type(v3d::event::Type::Destination);
}

v3d::event::Event TextBox::event() const {
    return event_;
}

};  // namespace v3d::ui::component
