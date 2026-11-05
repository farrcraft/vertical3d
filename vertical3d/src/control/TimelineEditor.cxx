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
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
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
 * The steps the ruler counts frames in are these times a power of ten: 1, 2, 5, 10, 20, 50 and
 * so on.
 **/
constexpr std::array<std::int64_t, 3> MANTISSAS{ 1, 2, 5 };
constexpr std::int64_t DECADE = 10;

/**
 * @return a number from the ui as the nearest whole frame, or nothing when it is too far away
 *         to be one
 **/
std::optional<int> whole(float frame) {
    return v3d::type::toInteger(std::round(frame), { .minimum = -FRAME_LIMIT, .maximum = FRAME_LIMIT });
}

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
    Flow flow{ .room = ui->rowWidth(), .used = 0.0F };
    transport(ui, &flow);
    fields(ui, &state, &flow);

    const v3d::ui::Immediate::Surface rule = ruler(ui, &state);
    float bottom = rule.max.y;
    for (const boost::shared_ptr<scene::item::Item>& item : document_->scene()->selection()) {
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
void TimelineEditor::icons(const Icons& resolve) {
    icons_ = resolve;
}

/**
 **/
void TimelineEditor::shortcuts(const Shortcuts& find) {
    shortcuts_ = find;
}

/**
 **/
std::vector<TimelineEditor::Control> TimelineEditor::controls(bool playing) {
    using enum Action;
    return {
        { .action = First, .id = "first", .icon = "icon:first-frame", .tooltip = "First frame", .command = "timeline::playback::first" },
        { .action = PreviousKey, .id = "previous key", .icon = "icon:previous-key", .tooltip = "Previous key",
            .command = "timeline::playback::previous_key" },
        playing ? Control{ .action = Stop, .id = "play", .icon = "icon:stop", .tooltip = "Stop", .command = "timeline::playback::toggle" }
                : Control{ .action = Play, .id = "play", .icon = "icon:play", .tooltip = "Play", .command = "timeline::playback::toggle" },
        { .action = NextKey, .id = "next key", .icon = "icon:next-key", .tooltip = "Next key", .command = "timeline::playback::next_key" },
        { .action = Last, .id = "last", .icon = "icon:last-frame", .tooltip = "Last frame", .command = "timeline::playback::last" },
    };
}

/**
 **/
void TimelineEditor::place(v3d::ui::Immediate* ui, Flow* flow, float width, bool sized, float after) {
    const float spacing = ui->dressing().spacing;
    const float more = after > 0.0F ? spacing + after : 0.0F;
    if (flow->used > 0.0F && flow->used + spacing + width + more <= flow->room) {
        ui->sameLine();
        flow->used += spacing + width;
    } else {
        flow->used = width;
    }
    // a width set for a control that sizes itself would be left for the next one
    if (sized) {
        ui->nextItemWidth(width);
    }
}

/**
 **/
void TimelineEditor::transport(v3d::ui::Immediate* ui, Flow* flow) {
    const scene::Items selected = document_->scene()->selection();
    const bool playing = playback_->playing();
    for (const Control& control : controls(playing)) {
        place(ui, flow, ui->dressing().barHeight, false);
        // stop is drawn as a toggle that is on, since playback is under way
        const v3d::ui::Immediate::IconLook look{ .icon = icons_ ? icons_(control.icon) : v3d::ui::Image(), .tooltip = control.tooltip,
            .keys = shortcuts_ ? shortcuts_(control.command) : std::string(), .on = control.action == Action::Stop };
        if (!ui->iconButton(control.id, look)) {
            continue;
        }
        switch (control.action) {
            case Action::First:
                playback_->first();
                break;
            case Action::PreviousKey:
                playback_->nextKey(selected, false);
                break;
            case Action::Play:
                playback_->play();
                break;
            case Action::Stop:
                playback_->stop();
                break;
            case Action::NextKey:
                playback_->nextKey(selected, true);
                break;
            case Action::Last:
                playback_->last();
                break;
        }
    }
}

/**
 **/
std::string TimelineEditor::rateName(float rate) {
    return std::format("{:g} fps", rate);
}

/**
 **/
void TimelineEditor::fields(v3d::ui::Immediate* ui, State* state, Flow* flow) {
    const scene::Timeline& timeline = document_->scene()->timeline();
    if (!state->editing) {
        state->start = static_cast<float>(timeline.start);
        state->end = static_cast<float>(timeline.end);
    }
    // each field is as wide as its label and a frame number, so the row stays compact
    const v3d::ui::paint::Dressing& dress = ui->dressing();
    const auto fit = [&](const char* label) { return ui->measure(label) + ui->measure(FIGURES) + dress.spacing + (dress.padding * 2.0F); };
    float frame = static_cast<float>(timeline.frame);
    place(ui, flow, fit(FRAME), true);
    if (ui->dragFloat(FRAME, &frame, FRAME_SPEED)) {
        if (const std::optional<int> at = whole(frame)) {
            goTo(*at);
        }
    }
    // the range's two ends go on one row
    bool finished = false;
    for (const auto& [label, value, after] : { std::tuple<const char*, float*, float>{ RANGE, &state->start, fit(RANGE_END) },
        { RANGE_END, &state->end, 0.0F } }) {
        place(ui, flow, fit(label), true, after);
        if (ui->dragFloat(label, value, FRAME_SPEED)) {
            state->editing = true;
        }
        finished = finished || ui->itemFinished();
    }
    if (finished && state->editing) {
        state->editing = false;
        const std::optional<int> start = whole(state->start);
        const std::optional<int> end = whole(state->end);
        const document::Outcome outcome = start && end ? document_->timing(timeline.rate, *start, *end)
            : document::Outcome{ .done = false, .message = "a frame is a whole number within a million of 0" };
        if (report_) {
            report_(outcome);
        }
    }
    rates(ui, flow);
    bool on = timeline.autoKey;
    place(ui, flow, dress.lineHeight + dress.padding + ui->measure(AUTO_KEY), false);
    if (ui->checkbox(AUTO_KEY, &on) && report_) {
        report_(document_->autoKey(on));
    }
}

/**
 **/
void TimelineEditor::rates(v3d::ui::Immediate* ui, Flow* flow) {
    const scene::Timeline& timeline = document_->scene()->timeline();
    // the common rates, and the timeline's own where it is none of them
    std::vector<float> offered(RATES.begin(), RATES.end());
    if (std::ranges::find(offered, timeline.rate) == offered.end()) {
        offered.insert(std::ranges::upper_bound(offered, timeline.rate), timeline.rate);
    }
    std::vector<std::string> names;
    int chosen = 0;
    for (std::size_t index = 0; index < offered.size(); index++) {
        names.push_back(rateName(offered[index]));
        if (offered[index] == timeline.rate) {
            chosen = static_cast<int>(index);
        }
    }
    // as wide as its label, its widest rate and its drop mark, so no rate is cut short
    float widest = 0.0F;
    for (const std::string& name : names) {
        widest = std::max(widest, ui->measure(name));
    }
    const v3d::ui::paint::Dressing& dress = ui->dressing();
    place(ui, flow, ui->measure(RATE) + widest + dress.lineHeight + dress.spacing + (dress.padding * 2.0F), true);
    if (ui->combo(RATE, &chosen, names) && chosen >= 0 && static_cast<std::size_t>(chosen) < offered.size() && report_) {
        report_(document_->timing(offered[static_cast<std::size_t>(chosen)], timeline.start, timeline.end));
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
v3d::ui::Immediate::Surface TimelineEditor::row(v3d::ui::Immediate* ui, State* state, const boost::shared_ptr<scene::item::Item>& item) {
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
        const boost::shared_ptr<scene::item::Item> item = scene.item(key.item);
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
