/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "LineEdit.h"

#include <api/ui/paint/Text.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace v3d::ui::input {

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

LineEdit::LineEdit() noexcept :
    caret_(0),
    anchor_(0),
    limit_(0) {
}

void LineEdit::text(const std::string& value) {
    text_ = value;
    caret_ = text_.size();
    anchor_ = caret_;
}

std::string_view LineEdit::text() const noexcept {
    return text_;
}

std::size_t LineEdit::boundary(std::size_t offset) const noexcept {
    std::size_t at = offset > text_.size() ? text_.size() : offset;
    while (at > 0 && at < text_.size() && continuation(text_[at])) {
        at--;
    }
    return at;
}

std::size_t LineEdit::forward(std::size_t offset) const noexcept {
    std::size_t at = offset >= text_.size() ? text_.size() : offset + 1;
    while (at < text_.size() && continuation(text_[at])) {
        at++;
    }
    return at;
}

std::pair<std::size_t, std::size_t> LineEdit::run() const noexcept {
    return std::make_pair(std::min(caret_, anchor_), std::max(caret_, anchor_));
}

void LineEdit::caret(std::size_t offset, bool extend) {
    caret_ = boundary(offset);
    if (!extend) {
        anchor_ = caret_;
    }
}

std::size_t LineEdit::caret() const noexcept {
    return caret_;
}

std::size_t LineEdit::anchor() const noexcept {
    return anchor_;
}

void LineEdit::select(std::size_t from, std::size_t to) {
    anchor_ = boundary(from);
    caret_ = boundary(to);
}

void LineEdit::selectAll() {
    anchor_ = 0;
    caret_ = text_.size();
}

void LineEdit::deselect() noexcept {
    anchor_ = caret_;
}

bool LineEdit::selected() const noexcept {
    return anchor_ != caret_;
}

std::string_view LineEdit::selection() const noexcept {
    const std::pair<std::size_t, std::size_t> span = run();
    return std::string_view(text_).substr(span.first, span.second - span.first);
}

bool LineEdit::removeSelection() {
    if (!selected()) {
        return false;
    }
    const std::pair<std::size_t, std::size_t> span = run();
    text_.erase(span.first, span.second - span.first);
    caret_ = span.first;
    anchor_ = caret_;
    return true;
}

void LineEdit::limit(std::size_t bytes) noexcept {
    limit_ = bytes;
}

std::size_t LineEdit::limit() const noexcept {
    return limit_;
}

bool LineEdit::insert(std::string_view value) {
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

bool LineEdit::backspace() {
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

bool LineEdit::erase() {
    if (removeSelection()) {
        return true;
    }
    if (caret_ >= text_.size()) {
        return false;
    }
    text_.erase(caret_, forward(caret_) - caret_);
    return true;
}

bool LineEdit::left(bool extend) {
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

bool LineEdit::right(bool extend) {
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

bool LineEdit::home(bool extend) {
    const bool moved = caret_ != 0 || (!extend && selected());
    caret_ = 0;
    if (!extend) {
        anchor_ = caret_;
    }
    return moved;
}

bool LineEdit::end(bool extend) {
    const bool moved = caret_ != text_.size() || (!extend && selected());
    caret_ = text_.size();
    if (!extend) {
        anchor_ = caret_;
    }
    return moved;
}

std::size_t LineEdit::at(float into, const paint::Measure& measure) const {
    if (!measure || text_.empty() || into <= 0.0F) {
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

namespace {

/**
 * Carry out a control chord on a line: copy, cut, paste and select all, and no other. Any other
 * chord goes on to the app, so a ctrl-s still saves while the line has the focus.
 *
 * @return whether the chord is one of the four, which is taken whether or not it changed
 *      anything, so it never also reaches a binding
 **/
bool chord(LineEdit* line, std::string_view key, const Clipboard& clipboard) {
    if (key == "c") {
        if (clipboard.write && line->selected()) {
            // an empty copy never replaces what the clipboard holds
            clipboard.write(line->selection());
        }
    } else if (key == "x") {
        // the run is written to the clipboard before it is removed, and is not removed at all
        // when there is no write callback, so a cut never loses text
        if (clipboard.write && line->selected()) {
            clipboard.write(line->selection());
            line->removeSelection();
        }
    } else if (key == "v") {
        if (clipboard.read) {
            // insert() replaces the selection, so a paste and a typed character land the same
            // way
            line->insert(clipboard.read());
        }
    } else if (key == "a") {
        line->selectAll();
    } else {
        return false;
    }
    return true;
}

};  // namespace

bool edit(LineEdit* line, std::string_view key, bool shifted, bool controlled, const Clipboard& clipboard) {
    if (line == nullptr) {
        return false;
    }
    if (controlled) {
        return chord(line, key, clipboard);
    }

    if (key == "backspace") {
        line->backspace();
    } else if (key == "delete") {
        line->erase();
    } else if (key == "arrow_left") {
        // shift keeps the anchor where it is, so the run travelled ends up selected
        line->left(shifted);
    } else if (key == "arrow_right") {
        line->right(shifted);
    } else if (key == "home") {
        line->home(shifted);
    } else if (key == "end") {
        line->end(shifted);
    } else if (key.size() == 1 || key == "space") {
        // a key that will arrive again as a character is taken here as well, so it does not
        // also reach the app's bindings. Typing "w" into a line must not walk the player
        // forward. api/input gives a key that composes text a one-character name, and any
        // other key a word, so the length of the name is the test
        return true;
    } else {
        // a function key, a modifier or a page down does nothing to the line, so the app
        // still receives it
        return false;
    }
    return true;
}

};  // namespace v3d::ui::input
