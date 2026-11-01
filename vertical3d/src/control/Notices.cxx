/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Notices.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>

namespace v3d::editor::control {

/**
 **/
void Notices::post(const Note& note) {
    if (note.text.empty()) {
        return;
    }
    text_ = note.text;
    done_ = note.done;
    undoable_ = note.undoable;
    text_.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(text_.front())));
    start_.reset();
}

/**
 **/
std::optional<Notices::Shown> Notices::shown(float now) {
    if (text_.empty()) {
        return std::nullopt;
    }
    if (!start_) {
        start_ = now;
    }
    const float age = now - *start_;
    if (age >= SHOWN) {
        text_.clear();
        start_.reset();
        return std::nullopt;
    }
    const float in = age / FADE;
    const float out = (SHOWN - age) / FADE;
    return Shown{ .text = text_, .alpha = std::clamp(std::min(in, out), 0.0F, 1.0F), .done = done_, .undoable = undoable_ };
}

};  // namespace v3d::editor::control
