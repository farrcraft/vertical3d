/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextBox.h"

#include <api/event/Event.h>
#include <api/event/Type.h>
#include <api/ui/Component.h>
#include <api/ui/input/LineEdit.h>
#include <api/ui/paint/Text.h>

#include "Type.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace v3d::ui::component {

TextBox::TextBox() :
    Component(component::Type::TextBox),
    pen_(0.0F) {
    // a control is pickable and focusable from the start. A plain component is neither, so a
    // panel laid over a scene lets presses through
    pickable(true);
    focusable(true);
}

void TextBox::text(const std::string& value) {
    line_.text(value);
}

std::string_view TextBox::text() const noexcept {
    return line_.text();
}

void TextBox::caret(std::size_t offset, bool extend) {
    line_.caret(offset, extend);
}

std::size_t TextBox::caret() const noexcept {
    return line_.caret();
}

std::size_t TextBox::anchor() const noexcept {
    return line_.anchor();
}

void TextBox::select(std::size_t from, std::size_t to) {
    line_.select(from, to);
}

void TextBox::selectAll() {
    line_.selectAll();
}

void TextBox::deselect() noexcept {
    line_.deselect();
}

bool TextBox::selected() const noexcept {
    return line_.selected();
}

std::string_view TextBox::selection() const noexcept {
    return line_.selection();
}

bool TextBox::removeSelection() {
    return line_.removeSelection();
}

void TextBox::limit(std::size_t bytes) noexcept {
    line_.limit(bytes);
}

std::size_t TextBox::limit() const noexcept {
    return line_.limit();
}

void TextBox::placeholder(const std::string& value) {
    placeholder_ = value;
}

std::string_view TextBox::placeholder() const noexcept {
    return placeholder_;
}

bool TextBox::insert(std::string_view value) {
    return line_.insert(value);
}

bool TextBox::backspace() {
    return line_.backspace();
}

bool TextBox::erase() {
    return line_.erase();
}

bool TextBox::left(bool extend) {
    return line_.left(extend);
}

bool TextBox::right(bool extend) {
    return line_.right(extend);
}

bool TextBox::home(bool extend) {
    return line_.home(extend);
}

bool TextBox::end(bool extend) {
    return line_.end(extend);
}

std::size_t TextBox::at(const glm::vec2& point, const paint::Measure& measure) const {
    return line_.at(point.x - pen_, measure);
}

input::LineEdit* TextBox::line() noexcept {
    return &line_;
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
