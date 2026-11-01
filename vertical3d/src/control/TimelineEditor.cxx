/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TimelineEditor.h"

#include <api/animation/Curve.h>
#include <api/type/Checked.h>
#include <api/ui/Immediate.h>
#include <api/ui/paint/Dressing.h>
#include <vertical3d/src/control/Outliner.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "Playback.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

namespace {

/**
 * The furthest from frame 0 a frame the timeline is asked for may be.
 **/
constexpr std::int32_t FRAME_LIMIT = 1000000;

/**
 * @return a number from the ui as the nearest whole frame, or nothing when it is too far away
 *         to be one
 **/
std::optional<int> whole(float frame) {
    return v3d::type::toInteger(std::round(frame), { .minimum = -FRAME_LIMIT, .maximum = FRAME_LIMIT });
}

/**
 * The steps the ruler counts frames in are these times a power of ten: 1, 2, 5, 10, 20, 50 and
 * so on.
 **/
constexpr std::array<std::int64_t, 3> MANTISSAS{ 1, 2, 5 };
constexpr std::int64_t DECADE = 10;

/**
 * @return the smallest step that leaves room between numbers, or one as long as the range
 * @param perFrame how many pixels a frame takes
 * @param room how many pixels each number needs
 * @param frames how many frames the range holds
 **/
std::int64_t rulerStep(double perFrame, double room, std::int64_t frames) {
    for (std::int64_t decade = 1; decade < frames * DECADE; decade *= DECADE) {
        for (const std::int64_t mantissa : MANTISSAS) {
            const std::int64_t step = mantissa * decade;
            if (static_cast<double>(step) * perFrame >= room || step >= frames) {
                return step;
            }
        }
    }
    return std::max<std::int64_t>(frames, 1);
}

/**
 * The key nearest a point along a track, when one lies within reach of it.
 **/
std::optional<float> nearest(float x, const std::vector<float>& frames, const v3d::editor::control::TimelineEditor::Span& span, float reach) {
    std::optional<float> hit;
    float best = reach;
    for (const float frame : frames) {
        const float distance = std::fabs(span.at(frame) - x);
        if (distance <= best) {
            hit = frame;
            best = distance;
        }
    }
    return hit;
}

};  // namespace

/**
 **/
float TimelineEditor::Span::at(float frame) const {
    const float frames = static_cast<float>(end - start + 1);
    return left + ((frame - (static_cast<float>(start) - 0.5F)) / frames * (right - left));
}

/**
 **/
float TimelineEditor::Span::frame(float x) const {
    const float frames = static_cast<float>(end - start + 1);
    return static_cast<float>(start) - 0.5F + ((x - left) / (right - left) * frames);
}

/**
 **/
TimelineEditor::TimelineEditor(const boost::shared_ptr<document::Document>& document, const boost::shared_ptr<Playback>& playback,
    const Report& report) :
    document_(document),
    playback_(playback),
    report_(report) {
}

/**
 **/
void TimelineEditor::draw(v3d::ui::Immediate* ui, std::size_t editor, const glm::vec4& region) {
    const std::string id = "timeline " + std::to_string(editor);
    ui->region(id, { .position = glm::vec2(region.x, region.y), .size = glm::vec2(region.z, region.w) }, 1.0F);
    ui->pushId(id);
    prune();
    State& state = states_[editor];
    transport(ui);
    fields(ui, &state);

    const v3d::ui::Immediate::Surface rule = ruler(ui, &state);
    float bottom = rule.max.y;
    for (const boost::shared_ptr<scene::Item>& item : document_->scene()->selection()) {
        bottom = row(ui, &state, item).max.y;
    }
    // the playhead runs down the ruler and every row, at the current frame
    const Span across = span(rule, ui->dressing().lineHeight);
    const float x = across.at(static_cast<float>(document_->scene()->timeline().frame));
    if (x >= across.left && x <= across.right) {
        ui->line(glm::vec2(x, rule.min.y), glm::vec2(x, bottom), 2.0F, ui->dressing().highlight);
    }
    ui->popId();
    ui->endRegion();
}

/**
 **/
const std::vector<scene::FrameKey>& TimelineEditor::chosen() const noexcept {
    return chosen_;
}

/**
 **/
void TimelineEditor::deleteChosen() {
    const document::Outcome outcome = document_->deleteKeys(chosen_);
    if (outcome.done) {
        chosen_.clear();
    }
    if (report_) {
        report_(outcome);
    }
}

/**
 **/
void TimelineEditor::transport(v3d::ui::Immediate* ui) {
    const scene::Items selected = document_->scene()->selection();
    if (ui->button("First")) {
        playback_->first();
    }
    ui->sameLine();
    if (ui->button("Previous Key")) {
        playback_->nextKey(selected, false);
    }
    ui->sameLine();
    const bool playing = playback_->playing();
    if (ui->button(playing ? "Stop" : "Play")) {
        if (playing) {
            playback_->stop();
        } else {
            playback_->play();
        }
    }
    ui->sameLine();
    if (ui->button("Next Key")) {
        playback_->nextKey(selected, true);
    }
    ui->sameLine();
    if (ui->button("Last")) {
        playback_->last();
    }
}

/**
 **/
void TimelineEditor::fields(v3d::ui::Immediate* ui, State* state) {
    const scene::Timeline& timeline = document_->scene()->timeline();
    if (!state->editing) {
        state->start = static_cast<float>(timeline.start);
        state->end = static_cast<float>(timeline.end);
        state->rate = timeline.rate;
    }
    // four fields share what the check leaves of the row
    const v3d::ui::paint::Dressing& dress = ui->dressing();
    const float check = dress.lineHeight + dress.padding + ui->measure(AUTO_KEY);
    const float width = std::max(0.0F, (ui->rowWidth() - check - (dress.spacing * 4.0F)) / 4.0F);
    float frame = static_cast<float>(timeline.frame);
    ui->nextItemWidth(width);
    if (ui->dragFloat("Frame", &frame, FRAME_SPEED)) {
        if (const std::optional<int> at = whole(frame)) {
            goTo(*at);
        }
    }
    bool finished = false;
    for (const auto& [label, value, speed] : { std::tuple<const char*, float*, float>{ "Start", &state->start, FRAME_SPEED },
        { "End", &state->end, FRAME_SPEED }, { "Rate", &state->rate, RATE_SPEED } }) {
        ui->sameLine();
        ui->nextItemWidth(width);
        if (ui->dragFloat(label, value, speed)) {
            state->editing = true;
        }
        finished = finished || ui->itemFinished();
    }
    if (finished && state->editing) {
        state->editing = false;
        const std::optional<int> start = whole(state->start);
        const std::optional<int> end = whole(state->end);
        const document::Outcome outcome = start && end ? document_->timing(state->rate, *start, *end)
            : document::Outcome{ .done = false, .message = "a frame is a whole number within a million of 0" };
        if (report_) {
            report_(outcome);
        }
    }
    bool on = timeline.autoKey;
    ui->sameLine();
    if (ui->checkbox(AUTO_KEY, &on) && report_) {
        report_(document_->autoKey(on));
    }
}

/**
 **/
v3d::ui::Immediate::Surface TimelineEditor::ruler(v3d::ui::Immediate* ui, State* state) {
    const v3d::ui::paint::Dressing& dress = ui->dressing();
    const v3d::ui::Immediate::Surface rule = ui->surface("ruler", dress.lineHeight);
    const Span across = span(rule, dress.lineHeight);
    const float margin = dress.lineHeight * KEY;
    ui->fill(glm::vec2(across.left - margin, rule.min.y), glm::vec2(across.right + margin, rule.max.y), dress.track);

    // the numbers are counted in the smallest step that leaves room for two lines' width each
    const std::int64_t frames = static_cast<std::int64_t>(across.end) - across.start + 1;
    const double perFrame = static_cast<double>(across.right - across.left) / static_cast<double>(frames);
    const std::int64_t step = rulerStep(perFrame, static_cast<double>(dress.lineHeight) * 2.0, frames);
    const auto label = [&](std::int64_t frame) {
        const float x = across.at(static_cast<float>(frame));
        ui->line(glm::vec2(x, rule.min.y + (dress.lineHeight * TICK)), glm::vec2(x, rule.max.y), 1.0F, dress.rule);
        ui->textAt(std::to_string(frame), glm::vec2(x + 2.0F, rule.min.y), dress.secondaryText);
    };
    // the first frame, then each multiple of the step at least half a step past it
    label(across.start);
    const std::int64_t after = across.start + ((step + 1) / 2);
    const std::int64_t first = after >= 0 ? ((after + step - 1) / step) * step : -((-after / step) * step);
    for (std::int64_t frame = first; frame <= across.end; frame += step) {
        label(frame);
    }

    if (rule.pressed) {
        chosen_.clear();
        state->drag = Drag::Scrub;
    }
    if (rule.held) {
        follow(rule, across, state);
    }
    return rule;
}

/**
 **/
v3d::ui::Immediate::Surface TimelineEditor::row(v3d::ui::Immediate* ui, State* state, const boost::shared_ptr<scene::Item>& item) {
    const v3d::ui::paint::Dressing& dress = ui->dressing();
    const v3d::ui::Immediate::Surface track = ui->surface("row " + std::to_string(item->id()), dress.lineHeight);
    const Span across = span(track, dress.lineHeight);
    const float radius = dress.lineHeight * KEY;
    ui->fill(glm::vec2(across.left - radius, track.min.y + 1.0F), glm::vec2(across.right + radius, track.max.y - 1.0F), dress.widget);
    ui->textAt(Outliner::label(*item), glm::vec2(track.min.x, track.min.y), dress.text);

    const float middle = (track.min.y + track.max.y) * 0.5F;
    const std::vector<float> frames = document_->scene()->keyedFrames({ item });
    for (const float frame : frames) {
        const bool chosen = isChosen({ .item = item->id(), .frame = frame });
        const float shown = frame + (chosen && state->drag == Drag::Move ? static_cast<float>(state->offset) : 0.0F);
        const float x = across.at(shown);
        if (x >= across.left && x <= across.right) {
            ui->diamond(glm::vec2(x, middle), radius, chosen ? CHOSEN : dress.text);
        }
    }

    if (track.pressed) {
        state->offset = 0;
        state->drag = Drag::None;
        if (track.cursor.x >= across.left) {
            press(item->id(), nearest(track.cursor.x, frames, across, radius + (dress.lineHeight * KEY_SLACK)), state);
        }
    }
    if (track.held) {
        follow(track, across, state);
    }
    return track;
}

/**
 **/
void TimelineEditor::press(unsigned int item, const std::optional<float>& hit, State* state) {
    if (!hit) {
        chosen_.clear();
        state->drag = Drag::Scrub;
        return;
    }
    const scene::FrameKey key{ .item = item, .frame = *hit };
    if (!isChosen(key)) {
        chosen_ = { key };
    }
    state->drag = Drag::Move;
}

/**
 **/
void TimelineEditor::follow(const v3d::ui::Immediate::Surface& surface, const Span& span, State* state) {
    if (state->drag == Drag::Scrub) {
        if (const std::optional<int> at = whole(span.frame(surface.cursor.x))) {
            goTo(*at);
        }
    } else if (state->drag == Drag::Move) {
        state->offset = whole(span.frame(surface.cursor.x) - span.frame(surface.pressAt.x)).value_or(0);
    }
    if (!surface.released) {
        return;
    }
    if (state->drag == Drag::Move && state->offset != 0) {
        const document::Outcome outcome = document_->moveKeys(chosen_, state->offset);
        if (outcome.done) {
            for (scene::FrameKey& key : chosen_) {
                key.frame += static_cast<float>(state->offset);
            }
        }
        if (report_) {
            report_(outcome);
        }
    }
    state->drag = Drag::None;
    state->offset = 0;
}

/**
 **/
void TimelineEditor::goTo(int frame) const {
    if (frame != document_->scene()->timeline().frame) {
        document_->goTo(frame);
    }
}

/**
 **/
TimelineEditor::Span TimelineEditor::span(const v3d::ui::Immediate::Surface& surface, float line) const {
    const scene::Timeline& timeline = document_->scene()->timeline();
    const float margin = line * KEY;
    const float left = std::min(surface.min.x + (line * NAMES) + margin, surface.max.x - 1.0F);
    return { .left = left, .right = std::max(surface.max.x - margin, left + 1.0F), .start = timeline.start, .end = timeline.end };
}

/**
 **/
bool TimelineEditor::isChosen(const scene::FrameKey& key) const {
    return std::any_of(chosen_.begin(), chosen_.end(), [&key](const scene::FrameKey& each) {
        return each.item == key.item && std::fabs(each.frame - key.frame) <= v3d::animation::Curve::SAME_FRAME;
    });
}

/**
 **/
void TimelineEditor::prune() {
    const scene::Scene& scene = *document_->scene();
    std::erase_if(chosen_, [&scene](const scene::FrameKey& key) {
        const boost::shared_ptr<scene::Item> item = scene.item(key.item);
        if (!item || !item->selected()) {
            return true;
        }
        const std::vector<float> frames = scene.keyedFrames({ item });
        return std::none_of(frames.begin(), frames.end(), [&key](float frame) {
            return std::fabs(frame - key.frame) <= v3d::animation::Curve::SAME_FRAME;
        });
    });
}

};  // namespace v3d::editor::control
