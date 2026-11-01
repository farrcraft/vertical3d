/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "GraphEditor.h"

#include <api/animation/Curve.h>
#include <api/type/Checked.h>
#include <api/ui/Immediate.h>
#include <api/ui/paint/Dressing.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>
#include <vertical3d/src/view/ViewOverlay.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

namespace {

/**
 * A line between two points.
 **/
using Line = std::array<glm::vec2, 2>;

/**
 * The furthest from frame 0 a frame the graph is asked for may be.
 **/
constexpr std::int32_t FRAME_LIMIT = 1000000;

/**
 * The fewest and the most frames, and the least and the most value, a view shows.
 **/
constexpr float FEWEST_FRAMES = 2.0F;
constexpr float MOST_FRAMES = 2.0F * static_cast<float>(FRAME_LIMIT);
constexpr float LEAST_VALUE = 1e-3F;
constexpr float MOST_VALUE = 1e7F;

/**
 * How far past the curves a fitted view reaches, against what they span, and the fewest frames
 * it shows.
 **/
constexpr float FIT_MARGIN = 0.1F;
constexpr float FIT_FRAMES = 10.0F;

/**
 * How many pixels each straight piece of a cubic is drawn across, and the most pieces one
 * segment is drawn in.
 **/
constexpr float PIECE = 4.0F;
constexpr std::uint32_t MOST_PIECE_COUNT = 64;
constexpr float MOST_PIECES = static_cast<float>(MOST_PIECE_COUNT);

/**
 * The steps the grid counts in are these times a power of ten: 1, 2, 5, 10, 20, 50 and so on.
 **/
constexpr std::array<double, 4> MANTISSAS{ 1.0, 2.0, 5.0, 10.0 };

/**
 * How wide a curve is drawn.
 **/
constexpr float CURVE_WIDTH = 1.5F;

/**
 * The most lines the grid draws each way, which no step that leaves room reaches.
 **/
constexpr int MOST_LINES = 1000;

/**
 * The most places a value line's number is written to.
 **/
constexpr double MOST_PLACES = 6.0;

/**
 * @return a number from the ui as the nearest whole frame, or nothing when it is too far away
 *         to be one
 **/
std::optional<int> whole(float frame) {
    return v3d::type::toInteger(std::round(frame), { .minimum = -FRAME_LIMIT, .maximum = FRAME_LIMIT });
}

/**
 * @return the smallest of 1, 2 or 5 times a power of ten that leaves room between lines, and is
 *         no smaller than the least step
 * @param perUnit how many pixels a unit takes
 * @param room how many pixels each line needs
 **/
double gridStep(double perUnit, double room, double least) {
    const double wanted = std::max(room / perUnit, least);
    if (!std::isfinite(wanted) || wanted <= 0.0) {
        return least;
    }
    const double decade = std::pow(10.0, std::floor(std::log10(wanted)));
    for (const double mantissa : MANTISSAS) {
        if (mantissa * decade >= wanted) {
            return mantissa * decade;
        }
    }
    return MANTISSAS.back() * decade;
}

/**
 * Call a function with each multiple of a step from one number to another.
 **/
template <typename Each>
void multiples(double first, double last, double step, const Each& each) {
    const double lowest = std::ceil(first / step);
    for (int index = 0; index < MOST_LINES; index++) {
        const double at = (lowest + index) * step;
        if (at > last) {
            break;
        }
        each(at);
    }
}

/**
 * @return a value line's number, to as many places as its step needs
 **/
std::string number(double value, double step) {
    const double places = std::clamp(-std::floor(std::log10(step)), 0.0, MOST_PLACES);
    // a multiple of the step that is all but 0 is written as 0, not as -0
    const double shown = std::fabs(value) < step * 1e-3 ? 0.0 : value;
    return std::format("{:.{}f}", shown, static_cast<int>(places));
}

/**
 * Cut a line to the part of it inside a box.
 * @return whether any of it is inside
 **/
bool clip(Line* line, const glm::vec2& min, const glm::vec2& max) {
    const glm::vec2 start = (*line)[0];
    const glm::vec2 along = (*line)[1] - start;
    float enter = 0.0F;
    float leave = 1.0F;
    // each edge as how fast the line crosses it and how far inside it the line starts
    const std::array<std::pair<float, float>, 4> edges{ { { -along.x, start.x - min.x }, { along.x, max.x - start.x },
        { -along.y, start.y - min.y }, { along.y, max.y - start.y } } };
    for (const auto& [speed, inside] : edges) {
        if (speed == 0.0F) {
            if (inside < 0.0F) {
                return false;
            }
            continue;
        }
        const float crossing = inside / speed;
        if (speed < 0.0F) {
            enter = std::max(enter, crossing);
        } else {
            leave = std::min(leave, crossing);
        }
    }
    if (enter > leave) {
        return false;
    }
    *line = { start + (along * enter), start + (along * leave) };
    return true;
}

/**
 * @return the index of a curve's key on a frame, or nothing when it has none there
 **/
std::optional<std::size_t> indexOf(const v3d::animation::Curve& curve, float frame) {
    const std::vector<v3d::animation::Curve::Key>& keys = curve.keys();
    for (std::size_t index = 0; index < keys.size(); index++) {
        if (std::fabs(keys[index].frame - frame) <= v3d::animation::Curve::SAME_FRAME) {
            return index;
        }
    }
    return std::nullopt;
}

/**
 * The frame and the value each handle of a key reaches. The in handle reaches a third of the way
 * to the key before, along the in slope. The out handle reaches a third of the way to the key
 * after, along the out slope. A key has a handle toward a neighbour only when the curve is a
 * cubic between them.
 **/
std::array<std::optional<glm::vec2>, 2> handles(const v3d::animation::Curve& curve, std::size_t index) {
    using Interpolation = v3d::animation::Curve::Interpolation;
    const std::vector<v3d::animation::Curve::Key>& keys = curve.keys();
    std::array<std::optional<glm::vec2>, 2> made;
    const v3d::animation::Curve::Key& key = keys[index];
    if (index > 0 && keys[index - 1].interpolation == Interpolation::Bezier) {
        const float reach = (key.frame - keys[index - 1].frame) / 3.0F;
        made[0] = glm::vec2(key.frame - reach, key.value - (key.in * reach));
    }
    if (index + 1 < keys.size() && key.interpolation == Interpolation::Bezier) {
        const float reach = (keys[index + 1].frame - key.frame) / 3.0F;
        made[1] = glm::vec2(key.frame + reach, key.value + (key.out * reach));
    }
    return made;
}

/**
 * Draw a line between two frames and values, cut to the graph.
 **/
void segment(v3d::ui::Immediate* ui, const GraphEditor::View& view, const glm::vec2& first, const glm::vec2& last, const glm::vec4& colour) {
    Line drawn{ view.at(first), view.at(last) };
    if (clip(&drawn, view.min, view.max)) {
        ui->line(drawn[0], drawn[1], CURVE_WIDTH, colour);
    }
}

/**
 * Draw a curve across the graph: held before its first key and after its last, and between
 * each two keys as the first one leaves for the next.
 **/
void trace(v3d::ui::Immediate* ui, const GraphEditor::View& view, const v3d::animation::Curve& curve, const glm::vec4& colour) {
    const std::vector<v3d::animation::Curve::Key>& keys = curve.keys();
    if (keys.empty()) {
        return;
    }
    const float left = view.frame;
    const float right = view.frame + view.frames;
    segment(ui, view, glm::vec2(std::min(left, keys.front().frame), keys.front().value), glm::vec2(keys.front().frame, keys.front().value), colour);
    for (std::size_t index = 0; index + 1 < keys.size(); index++) {
        const v3d::animation::Curve::Key& from = keys[index];
        const v3d::animation::Curve::Key& to = keys[index + 1];
        if (to.frame < left || from.frame > right) {
            continue;
        }
        switch (from.interpolation) {
            case v3d::animation::Curve::Interpolation::Step:
                segment(ui, view, glm::vec2(from.frame, from.value), glm::vec2(to.frame, from.value), colour);
                segment(ui, view, glm::vec2(to.frame, from.value), glm::vec2(to.frame, to.value), colour);
                break;
            case v3d::animation::Curve::Interpolation::Linear:
                segment(ui, view, glm::vec2(from.frame, from.value), glm::vec2(to.frame, to.value), colour);
                break;
            case v3d::animation::Curve::Interpolation::Bezier: {
                const float wide = (to.frame - from.frame) / view.frames * (view.max.x - view.min.x);
                const std::uint32_t pieces = v3d::type::toCount(std::clamp(wide / PIECE, 1.0F, MOST_PIECES), MOST_PIECE_COUNT).value_or(1U);
                glm::vec2 previous(from.frame, from.value);
                for (std::uint32_t piece = 1; piece <= pieces; piece++) {
                    const float frame = from.frame + ((to.frame - from.frame) * static_cast<float>(piece) / static_cast<float>(pieces));
                    const glm::vec2 next(frame, piece == pieces ? to.value : curve.sample(frame));
                    segment(ui, view, previous, next, colour);
                    previous = next;
                }
                break;
            }
        }
    }
    segment(ui, view, glm::vec2(keys.back().frame, keys.back().value), glm::vec2(std::max(right, keys.back().frame), keys.back().value), colour);
}

/**
 * @return whether a point is inside the graph
 **/
bool inside(const GraphEditor::View& view, const glm::vec2& at) {
    return at.x >= view.min.x && at.x <= view.max.x && at.y >= view.min.y && at.y <= view.max.y;
}

/**
 * Draw the grid of frames and values, with each value line numbered.
 **/
void grid(v3d::ui::Immediate* ui, const GraphEditor::View& view) {
    const v3d::ui::paint::Dressing& dress = ui->dressing();
    const double room = static_cast<double>(dress.lineHeight) * 2.0;
    const double perFrame = static_cast<double>(view.max.x - view.min.x) / static_cast<double>(view.frames);
    multiples(view.frame, view.frame + view.frames, gridStep(perFrame, room, 1.0), [&](double frame) {
        const float x = view.at(glm::vec2(static_cast<float>(frame), 0.0F)).x;
        ui->line(glm::vec2(x, view.min.y), glm::vec2(x, view.max.y), 1.0F, dress.rule);
    });
    const double perValue = static_cast<double>(view.max.y - view.min.y) / static_cast<double>(view.values);
    const double step = gridStep(perValue, room, static_cast<double>(LEAST_VALUE));
    multiples(view.value, view.value + view.values, step, [&](double value) {
        const float y = view.at(glm::vec2(0.0F, static_cast<float>(value))).y;
        ui->line(glm::vec2(view.min.x, y), glm::vec2(view.max.x, y), 1.0F, dress.rule);
        // a number sits above its line, unless that would put it above the graph
        if (y - dress.lineHeight >= view.min.y) {
            ui->textAt(number(value, step), glm::vec2(view.min.x + 2.0F, y - dress.lineHeight), dress.secondaryText);
        }
    });
}

};  // namespace

/**
 **/
glm::vec2 GraphEditor::View::at(const glm::vec2& place) const {
    return glm::vec2(min.x + ((place.x - frame) / frames * (max.x - min.x)), max.y - ((place.y - value) / values * (max.y - min.y)));
}

/**
 **/
glm::vec2 GraphEditor::View::point(const glm::vec2& at) const {
    const float wide = std::max(max.x - min.x, 1.0F);
    const float tall = std::max(max.y - min.y, 1.0F);
    return glm::vec2(frame + ((at.x - min.x) / wide * frames), value + ((max.y - at.y) / tall * values));
}

/**
 **/
void GraphEditor::View::zoom(const glm::vec2& about, const Scale& scale) {
    const glm::vec2 held = point(about);
    frames = std::clamp(frames * scale.frames, FEWEST_FRAMES, MOST_FRAMES);
    values = std::clamp(values * scale.values, LEAST_VALUE, MOST_VALUE);
    const glm::vec2 moved = point(about);
    frame += held.x - moved.x;
    value += held.y - moved.y;
}

/**
 **/
GraphEditor::GraphEditor(const boost::shared_ptr<document::Document>& document, const Report& report) :
    document_(document),
    report_(report) {
}

/**
 **/
void GraphEditor::draw(v3d::ui::Immediate* ui, std::size_t editor, const glm::vec4& region, Navigate navigate) {
    const std::string id = "graph " + std::to_string(editor);
    ui->region(id, { .position = glm::vec2(region.x, region.y), .size = glm::vec2(region.z, region.w) }, 1.0F);
    ui->pushId(id);
    prune();
    State& state = states_[editor];
    if (ui->button("Frame All")) {
        state.fit = true;
    }
    const v3d::ui::paint::Dressing& dress = ui->dressing();
    const v3d::ui::Immediate::Surface rule = ui->surface("ruler", dress.lineHeight);
    const v3d::ui::Immediate::Surface plot = ui->surface("plot", ui->roomBelow());
    state.view.min = plot.min;
    state.view.max = plot.max;
    if (state.fit && plot.max.x > plot.min.x && plot.max.y > plot.min.y) {
        fit(&state.view);
        state.fit = false;
    }

    if (plot.pressed && state.drag == Drag::None) {
        press(plot, dress.lineHeight, navigate, &state);
    }
    if (plot.held) {
        follow(plot, &state);
    } else if (plot.wheel != 0.0F) {
        const float shown = std::pow(WHEEL_ZOOM, -plot.wheel);
        state.view.zoom(plot.cursor, { .frames = shown, .values = shown });
    }

    ui->fill(plot.min, plot.max, dress.track);
    grid(ui, state.view);
    curves(ui, state);
    ruler(ui, rule, &state);

    // the playhead runs down the ruler and the graph, at the current frame
    const float x = state.view.at(glm::vec2(static_cast<float>(document_->scene()->timeline().frame), 0.0F)).x;
    if (x >= plot.min.x && x <= plot.max.x) {
        ui->line(glm::vec2(x, rule.min.y), glm::vec2(x, plot.max.y), 2.0F, dress.highlight);
    }
    ui->popId();
    ui->endRegion();
}

/**
 **/
std::optional<GraphEditor::View> GraphEditor::view(std::size_t editor) const {
    const auto found = states_.find(editor);
    if (found == states_.end()) {
        return std::nullopt;
    }
    return found->second.view;
}

/**
 **/
const std::optional<GraphEditor::CurveKey>& GraphEditor::chosen() const noexcept {
    return chosen_;
}

/**
 **/
glm::vec4 GraphEditor::colour(const std::string& path, const glm::vec4& otherwise) {
    if (path.ends_with(".x")) {
        return view::OverlayLook::X_AXIS;
    }
    if (path.ends_with(".y")) {
        return view::OverlayLook::Y_AXIS;
    }
    if (path.ends_with(".z")) {
        return view::OverlayLook::Z_AXIS;
    }
    return otherwise;
}

/**
 **/
void GraphEditor::ruler(v3d::ui::Immediate* ui, const v3d::ui::Immediate::Surface& rule, State* state) {
    const v3d::ui::paint::Dressing& dress = ui->dressing();
    const View& view = state->view;
    ui->fill(rule.min, rule.max, dress.widget);
    const double perFrame = static_cast<double>(view.max.x - view.min.x) / static_cast<double>(view.frames);
    const double step = gridStep(perFrame, static_cast<double>(dress.lineHeight) * 2.0, 1.0);
    multiples(view.frame, view.frame + view.frames, step, [&](double frame) {
        const float x = view.at(glm::vec2(static_cast<float>(frame), 0.0F)).x;
        if (const std::optional<int> label = whole(static_cast<float>(frame))) {
            ui->textAt(std::to_string(*label), glm::vec2(x + 2.0F, rule.min.y), dress.secondaryText);
        }
    });

    if (rule.pressed) {
        state->drag = Drag::Scrub;
    }
    if (rule.held && state->drag == Drag::Scrub) {
        const std::optional<int> at = whole(view.point(rule.cursor).x);
        if (at && *at != document_->scene()->timeline().frame) {
            document_->goTo(*at);
        }
        if (rule.released) {
            state->drag = Drag::None;
        }
    }
}

/**
 **/
void GraphEditor::curves(v3d::ui::Immediate* ui, const State& state) const {
    const v3d::ui::paint::Dressing& dress = ui->dressing();
    const View& view = state.view;
    const scene::Scene& scene = *document_->scene();
    for (const boost::shared_ptr<scene::Item>& item : scene.selection()) {
        for (const auto& [path, kept] : scene.curves(item->id())) {
            const bool holds = chosen_ && chosen_->item == item->id() && chosen_->path == path;
            const v3d::animation::Curve& curve = holds && state.draft ? *state.draft : kept;
            trace(ui, view, curve, colour(path, dress.text));
            for (const v3d::animation::Curve::Key& key : curve.keys()) {
                const glm::vec2 at = view.at(glm::vec2(key.frame, key.value));
                if (inside(view, at)) {
                    ui->diamond(at, dress.lineHeight * KEY, dress.text);
                }
            }
        }
    }
    drawChosen(ui, state);
}

/**
 **/
void GraphEditor::drawChosen(v3d::ui::Immediate* ui, const State& state) const {
    if (!chosen_) {
        return;
    }
    const std::optional<v3d::animation::Curve> chosen = state.draft ? state.draft : chosenCurve();
    const float frame = state.draft && state.drag == Drag::Key ? state.landing : chosen_->frame;
    const std::optional<std::size_t> index = chosen ? indexOf(*chosen, frame) : std::nullopt;
    if (!chosen || !index) {
        return;
    }
    const v3d::ui::paint::Dressing& dress = ui->dressing();
    const View& view = state.view;
    const v3d::animation::Curve::Key& key = chosen->keys()[*index];
    for (const std::optional<glm::vec2>& handle : handles(*chosen, *index)) {
        if (handle) {
            segment(ui, view, glm::vec2(key.frame, key.value), *handle, CHOSEN);
            const glm::vec2 end = view.at(*handle);
            if (inside(view, end)) {
                ui->diamond(end, dress.lineHeight * HANDLE, CHOSEN);
            }
        }
    }
    const glm::vec2 at = view.at(glm::vec2(key.frame, key.value));
    if (inside(view, at)) {
        ui->diamond(at, dress.lineHeight * KEY, CHOSEN);
    }
}

/**
 **/
void GraphEditor::press(const v3d::ui::Immediate::Surface& plot, float line, Navigate navigate, State* state) {
    state->last = plot.cursor;
    state->draft.reset();
    if (navigate != Navigate::None) {
        state->drag = navigate == Navigate::Pan ? Drag::Pan : Drag::Zoom;
        return;
    }
    // the chosen key's handles are over every key, so they are taken first
    if (!grab(plot.cursor, line * REACH, state)) {
        choose(plot.cursor, line * REACH, state);
    }
}

/**
 **/
bool GraphEditor::grab(const glm::vec2& cursor, float reach, State* state) const {
    if (!chosen_) {
        return false;
    }
    const std::optional<v3d::animation::Curve> curve = chosenCurve();
    const std::optional<std::size_t> index = curve ? indexOf(*curve, chosen_->frame) : std::nullopt;
    if (!curve || !index) {
        return false;
    }
    const std::array<std::optional<glm::vec2>, 2> ends = handles(*curve, *index);
    for (std::size_t side = 0; side < ends.size(); side++) {
        if (ends[side] && glm::distance(state->view.at(*ends[side]), cursor) <= reach) {
            state->original = *curve;
            state->drag = side == 0 ? Drag::In : Drag::Out;
            return true;
        }
    }
    return false;
}

/**
 **/
void GraphEditor::choose(const glm::vec2& cursor, float reach, State* state) {
    std::optional<CurveKey> hit;
    v3d::animation::Curve held;
    float best = reach;
    const scene::Scene& scene = *document_->scene();
    for (const boost::shared_ptr<scene::Item>& item : scene.selection()) {
        for (const auto& [path, curve] : scene.curves(item->id())) {
            for (const v3d::animation::Curve::Key& key : curve.keys()) {
                const float distance = glm::distance(state->view.at(glm::vec2(key.frame, key.value)), cursor);
                if (distance <= best) {
                    best = distance;
                    hit = CurveKey{ .item = item->id(), .path = path, .frame = key.frame };
                    held = curve;
                }
            }
        }
    }
    chosen_ = hit;
    if (hit) {
        state->original = held;
        state->landing = hit->frame;
        state->drag = Drag::Key;
    }
}

/**
 **/
void GraphEditor::follow(const v3d::ui::Immediate::Surface& plot, State* state) {
    View& view = state->view;
    const glm::vec2 moved = plot.cursor - state->last;
    state->last = plot.cursor;
    switch (state->drag) {
        case Drag::Pan:
            view.frame -= moved.x / std::max(view.max.x - view.min.x, 1.0F) * view.frames;
            view.value += moved.y / std::max(view.max.y - view.min.y, 1.0F) * view.values;
            break;
        case Drag::Zoom:
            // a drag right or up shows less
            view.zoom(plot.pressAt, { .frames = std::exp(-moved.x * DRAG_ZOOM), .values = std::exp(moved.y * DRAG_ZOOM) });
            break;
        case Drag::Key:
        case Drag::In:
        case Drag::Out:
            reshape(plot, state);
            break;
        case Drag::None:
        case Drag::Scrub:
            break;
    }
    if (plot.released) {
        commit(state);
    }
}

/**
 **/
void GraphEditor::reshape(const v3d::ui::Immediate::Surface& plot, State* state) const {
    const std::optional<std::size_t> index = chosen_ ? indexOf(state->original, chosen_->frame) : std::nullopt;
    if (!index) {
        return;
    }
    const v3d::animation::Curve::Key key = state->original.keys()[*index];
    const glm::vec2 to = state->view.point(plot.cursor);
    v3d::animation::Curve draft = state->original;
    if (state->drag == Drag::Key) {
        const glm::vec2 from = state->view.point(plot.pressAt);
        const std::optional<int> landing = whole(key.frame + to.x - from.x);
        state->landing = landing ? static_cast<float>(*landing) : key.frame;
        if (std::fabs(state->landing - key.frame) > v3d::animation::Curve::SAME_FRAME) {
            draft.shift({ key.frame }, state->landing - key.frame);
        }
        draft.set(state->landing, key.value + (to.y - from.y));
    } else {
        // a handle is held on its own side of the key, so its slope stays finite
        const bool out = state->drag == Drag::Out;
        const float along = out ? to.x - key.frame : key.frame - to.x;
        if (along > v3d::animation::Curve::SAME_FRAME) {
            const float slope = (out ? to.y - key.value : key.value - to.y) / along;
            draft.slopes(key.frame, { .in = slope, .out = slope });
        }
    }
    state->draft = draft;
}

/**
 **/
void GraphEditor::commit(State* state) {
    if (state->draft && chosen_ && *state->draft != state->original) {
        const document::Outcome outcome = document_->reshape(chosen_->item, chosen_->path, *state->draft);
        if (outcome.done && state->drag == Drag::Key) {
            chosen_->frame = state->landing;
        }
        if (report_) {
            report_(outcome);
        }
    }
    state->draft.reset();
    state->drag = Drag::None;
}

/**
 **/
void GraphEditor::fit(View* view) const {
    const scene::Scene& scene = *document_->scene();
    float first = std::numeric_limits<float>::max();
    float last = std::numeric_limits<float>::lowest();
    float lowest = std::numeric_limits<float>::max();
    float highest = std::numeric_limits<float>::lowest();
    for (const boost::shared_ptr<scene::Item>& item : scene.selection()) {
        for (const auto& [path, curve] : scene.curves(item->id())) {
            const std::vector<v3d::animation::Curve::Key>& keys = curve.keys();
            for (std::size_t index = 0; index < keys.size(); index++) {
                first = std::min(first, keys[index].frame);
                last = std::max(last, keys[index].frame);
                lowest = std::min(lowest, keys[index].value);
                highest = std::max(highest, keys[index].value);
                // a cubic given its own slopes can swing past its keys, so it is sampled between
                // them
                if (index + 1 < keys.size()) {
                    for (const float along : { 0.25F, 0.5F, 0.75F }) {
                        const float value = curve.sample(keys[index].frame + ((keys[index + 1].frame - keys[index].frame) * along));
                        lowest = std::min(lowest, value);
                        highest = std::max(highest, value);
                    }
                }
            }
        }
    }
    if (first > last) {
        const scene::Timeline& timeline = scene.timeline();
        first = static_cast<float>(timeline.start);
        last = static_cast<float>(timeline.end);
        lowest = -1.0F;
        highest = 1.0F;
    }
    const float frames = std::max(last - first, FIT_FRAMES);
    const float values = std::max(highest - lowest, 2.0F * LEAST_VALUE);
    const float middleFrame = (first + last) * 0.5F;
    const float middleValue = (lowest + highest) * 0.5F;
    view->frames = std::clamp(frames * (1.0F + (2.0F * FIT_MARGIN)), FEWEST_FRAMES, MOST_FRAMES);
    view->values = std::clamp(highest > lowest ? values * (1.0F + (2.0F * FIT_MARGIN)) : 2.0F, LEAST_VALUE, MOST_VALUE);
    view->frame = middleFrame - (view->frames * 0.5F);
    view->value = middleValue - (view->values * 0.5F);
}

/**
 **/
std::optional<v3d::animation::Curve> GraphEditor::chosenCurve() const {
    if (!chosen_) {
        return std::nullopt;
    }
    const scene::Curves curves = document_->scene()->curves(chosen_->item);
    const auto found = curves.find(chosen_->path);
    if (found == curves.end()) {
        return std::nullopt;
    }
    return found->second;
}

/**
 **/
void GraphEditor::prune() {
    if (!chosen_) {
        return;
    }
    const boost::shared_ptr<scene::Item> item = document_->scene()->item(chosen_->item);
    const std::optional<v3d::animation::Curve> curve = chosenCurve();
    if (!item || !item->selected() || !curve || !indexOf(*curve, chosen_->frame)) {
        chosen_.reset();
    }
}

};  // namespace v3d::editor::control
