/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ConsoleEditor.h"

#include <api/ui/Immediate.h>
#include <api/ui/paint/Dressing.h>
#include <vertical3d/src/script/Journal.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

namespace {

/**
 * The colour a refusal and a problem are set in, the design's red.
 **/
constexpr glm::vec4 REFUSED{ 0.851F, 0.349F, 0.373F, 1.0F };

/**
 * How many rows a notch of the wheel scrolls the list.
 **/
constexpr float NOTCH = 3.0F;

};  // namespace

/**
 **/
ConsoleEditor::ConsoleEditor(const boost::shared_ptr<script::Journal>& journal, const Run& run) :
    journal_(journal),
    run_(run) {
}

/**
 **/
void ConsoleEditor::draw(v3d::ui::Immediate* ui, std::size_t editor, const glm::vec4& region) {
    const std::string id = "console " + std::to_string(editor);
    ui->region(id, { .position = glm::vec2(region.x, region.y), .size = glm::vec2(region.z, region.w) }, 1.0F);
    ui->pushId(id);
    State& state = states_[editor];
    // an entry added since the console was last drawn brings the list back to its foot
    if (journal_ && journal_->revision() != state.seen) {
        state.seen = journal_->revision();
        state.back = 0.0F;
    }
    const float field = ui->dressing().barHeight + ui->dressing().spacing;
    const float room = ui->roomBelow() - field;
    if (room > 0.0F) {
        list(ui, &state, room);
    }
    if (state.keep) {
        ui->focusNext();
        state.keep = false;
    }
    ui->textField("Type a script, and press Return", &state.line);
    if (ui->itemTyping()) {
        typing_ = editor;
    } else if (typing_ == editor) {
        typing_.reset();
    }
    if (ui->itemEntered()) {
        enter(&state);
    }
    ui->popId();
    ui->endRegion();
}

/**
 **/
bool ConsoleEditor::typing() const noexcept {
    return typing_.has_value();
}

/**
 **/
void ConsoleEditor::recall(int step) {
    if (!typing_ || history_.empty() || step == 0) {
        return;
    }
    State& state = states_[*typing_];
    const std::size_t latest = history_.size();
    std::size_t at = state.recalled.value_or(latest);
    if (!state.recalled) {
        state.draft = state.line;
    }
    if (step < 0) {
        at = at > 0 ? at - 1 : 0;
    } else {
        at = std::min(at + 1, latest);
    }
    if (at == latest) {
        state.recalled.reset();
        state.line = state.draft;
    } else {
        state.recalled = at;
        state.line = history_[at];
    }
}

/**
 **/
std::string ConsoleEditor::line(std::size_t editor) const {
    const auto found = states_.find(editor);
    return found == states_.end() ? std::string() : found->second.line;
}

/**
 **/
std::vector<ConsoleEditor::Row> ConsoleEditor::rows(const v3d::ui::paint::Dressing& dressing) const {
    std::vector<Row> made;
    if (!journal_) {
        return made;
    }
    for (const script::Journal::Entry& entry : journal_->entries()) {
        std::string text = entry.text;
        glm::vec4 colour = dressing.text;
        switch (entry.kind) {
            case script::Journal::Kind::Typed:
                text.insert(0, "> ");
                colour = dressing.activeText;
                break;
            case script::Journal::Kind::Command:
                colour = dressing.secondaryText;
                break;
            case script::Journal::Kind::Refused:
            case script::Journal::Kind::Failed:
                colour = REFUSED;
                break;
            case script::Journal::Kind::Done:
            case script::Journal::Kind::Said:
                break;
        }
        // an entry over lines takes a row for each
        for (std::size_t start = 0;;) {
            const std::size_t end = text.find('\n', start);
            made.push_back({ .text = text.substr(start, end == std::string::npos ? std::string::npos : end - start), .colour = colour });
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
    }
    return made;
}

/**
 **/
void ConsoleEditor::list(v3d::ui::Immediate* ui, State* state, float height) const {
    const v3d::ui::Immediate::Surface log = ui->surface("list", height);
    const v3d::ui::paint::Dressing& dressing = ui->dressing();
    const std::vector<Row> shown = rows(dressing);
    const float row = dressing.lineHeight;
    const float fits = std::max(std::floor((log.max.y - log.min.y) / row), 1.0F);
    const float most = std::max(static_cast<float>(shown.size()) - fits, 0.0F);
    if (log.hovered && log.wheel != 0.0F) {
        state->back = std::clamp(state->back + (log.wheel * NOTCH), 0.0F, most);
    }
    state->back = std::min(state->back, most);
    // the newest row the list shows sits at its foot, and the rows before it rise from there
    const auto skipped = static_cast<std::size_t>(state->back);
    float bottom = log.max.y;
    for (std::size_t each = shown.size(); each > skipped && bottom - row >= log.min.y; --each) {
        bottom -= row;
        const Row& drawn = shown[each - 1 - skipped];
        ui->textAt(drawn.text, glm::vec2(log.min.x + dressing.padding, bottom), drawn.colour);
    }
}

/**
 **/
void ConsoleEditor::enter(State* state) {
    state->keep = true;
    state->recalled.reset();
    state->draft.clear();
    const std::string typed = state->line;
    if (typed.find_first_not_of(" \t") == std::string::npos) {
        state->line.clear();
        return;
    }
    if (history_.empty() || history_.back() != typed) {
        history_.push_back(typed);
    }
    state->line.clear();
    if (run_) {
        run_(typed);
    }
}

};  // namespace v3d::editor::control
