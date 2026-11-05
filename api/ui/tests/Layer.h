/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Immediate.h>
#include <api/ui/tests/Typeset.h>

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::layer {

/*
 * What the immediate layer's cases share: a layer that measures predictably, the input of a
 * frame, and a record of what the layer wrote.
 */

/**
 * A fixed width per character, so a widget's width, and where a press lands in a line, is
 * predictable.
 **/
const float characterWidth = v3d::ui::typeset::characterWidth;

/**
 * Where a field's line starts: the default padding in from the widget's left edge, which is
 * at the canvas's left edge outside any window.
 **/
const float lineStart = 6.0F;

/**
 * A point inside a widget drawn first on the canvas, which is the top row.
 **/
constexpr glm::vec2 firstRow(100.0F, 10.0F);

/**
 * A point well away from anything drawn.
 **/
constexpr glm::vec2 nowhere(390.0F, 290.0F);

/**
 * What the layer wrote, so a test can read a panel back without a device.
 **/
using Written = v3d::ui::typeset::Written;

/**
 * A layer that measures at ten pixels a character and records what it wrote.
 **/
inline v3d::ui::Immediate build(std::vector<Written>* written) {
    return v3d::ui::Immediate(v3d::ui::typeset::mono(written));
}

/**
 * A layer that measures at ten pixels a character.
 **/
inline v3d::ui::Immediate build() {
    return v3d::ui::Immediate(v3d::ui::typeset::mono());
}

/**
 * A press at a point, which is the first frame of a click.
 **/
inline v3d::ui::Immediate::Input press(const glm::vec2& at) {
    v3d::ui::Immediate::Input input;
    input.cursor = at;
    input.down = true;
    input.pressed = true;
    return input;
}

/**
 * The release that finishes one.
 **/
inline v3d::ui::Immediate::Input release(const glm::vec2& at) {
    v3d::ui::Immediate::Input input;
    input.cursor = at;
    input.released = true;
    return input;
}

/**
 * The cursor resting somewhere with nothing held.
 **/
inline v3d::ui::Immediate::Input hover(const glm::vec2& at) {
    v3d::ui::Immediate::Input input;
    input.cursor = at;
    return input;
}

/**
 * The wheel turned, with the cursor resting where it is.
 **/
inline v3d::ui::Immediate::Input wheel(const glm::vec2& at, float notches) {
    v3d::ui::Immediate::Input input;
    input.cursor = at;
    input.wheel = notches;
    return input;
}

/**
 * A press already down, resting where it has been dragged to.
 **/
inline v3d::ui::Immediate::Input hold(const glm::vec2& at) {
    v3d::ui::Immediate::Input input;
    input.cursor = at;
    input.down = true;
    return input;
}

/**
 * @return whether anything written carried this string
 **/
inline bool wrote(const std::vector<Written>& written, const std::string& text) {
    return std::any_of(written.begin(), written.end(),
        [&text](const Written& entry) { return entry.text == text; });
}

/**
 * Draw one frame of a panel under some input.
 **/
inline void frame(v3d::ui::Immediate* ui, v3d::render::realtime::canvas::Canvas* canvas,
    const v3d::ui::Immediate::Input& input, const std::function<void()>& panel) {
    ui->begin(canvas, input);
    panel();
    ui->end();
}

/**
 * A click: the frame that finds the cursor, the press, and the release.
 **/
inline void click(v3d::ui::Immediate* ui, v3d::render::realtime::canvas::Canvas* canvas, const glm::vec2& at,
    const std::function<void()>& panel) {
    frame(ui, canvas, hover(at), panel);
    frame(ui, canvas, press(at), panel);
    frame(ui, canvas, release(at), panel);
}

};  // namespace v3d::ui::layer
