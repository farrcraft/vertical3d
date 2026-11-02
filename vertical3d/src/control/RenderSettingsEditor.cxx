/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderSettingsEditor.h"

#include <api/type/Checked.h>
#include <api/ui/Immediate.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/RenderSettings.h>

#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

/**
 **/
RenderSettingsEditor::RenderSettingsEditor(const boost::shared_ptr<document::Document>& document, const Report& report) :
    document_(document),
    report_(report) {
}

/**
 **/
void RenderSettingsEditor::draw(v3d::ui::Immediate* ui, std::size_t editor, const glm::vec4& region) {
    const std::string id = "render settings " + std::to_string(editor);
    ui->region(id, { .position = glm::vec2(region.x, region.y), .size = glm::vec2(region.z, region.w) }, 1.0F);
    ui->pushId(id);
    State& state = states_[editor];
    const scene::RenderSettings& held = document_->scene()->renderSettings();
    if (!state.editing) {
        state.width = static_cast<float>(held.size.x);
        state.height = static_cast<float>(held.size.y);
        state.samples = static_cast<float>(held.samples);
    }
    bool finished = false;
    for (const auto& [label, value, speed] : { std::tuple<const char*, float*, float>{ "Width", &state.width, SIDE_SPEED },
        { "Height", &state.height, SIDE_SPEED }, { "Samples", &state.samples, SAMPLE_SPEED } }) {
        if (ui->dragFloat(label, value, speed)) {
            state.editing = true;
        }
        finished = finished || ui->itemFinished();
    }
    if (finished && state.editing) {
        state.editing = false;
        take(state);
    }

    int hider = held.hider == scene::RenderSettings::Hider::RayTrace ? 1 : 0;
    if (ui->combo("Hider", &hider, { "Reyes", "Ray trace" })) {
        scene::RenderSettings chosen = held;
        chosen.hider = hider == 1 ? scene::RenderSettings::Hider::RayTrace : scene::RenderSettings::Hider::Hidden;
        apply(chosen);
    }
    ui->popId();
    ui->endRegion();
}

/**
 **/
void RenderSettingsEditor::take(const State& state) const {
    const int largest = static_cast<int>(scene::RenderSettings::LARGEST_SIDE);
    const int most = static_cast<int>(scene::RenderSettings::MOST_SAMPLES);
    const std::optional<int> width = v3d::type::toInteger(std::round(state.width), { .minimum = 1, .maximum = largest });
    const std::optional<int> height = v3d::type::toInteger(std::round(state.height), { .minimum = 1, .maximum = largest });
    const std::optional<int> samples = v3d::type::toInteger(std::round(state.samples), { .minimum = 1, .maximum = most });
    if (!width || !height || !samples) {
        if (report_) {
            report_({ .done = false, .message = "the render settings need a width and a height of 1 to " + std::to_string(largest) +
                " and samples of 1 to " + std::to_string(most) });
        }
        return;
    }
    scene::RenderSettings settings = document_->scene()->renderSettings();
    settings.size = glm::uvec2(static_cast<unsigned int>(*width), static_cast<unsigned int>(*height));
    settings.samples = static_cast<unsigned int>(*samples);
    apply(settings);
}

/**
 **/
void RenderSettingsEditor::apply(const scene::RenderSettings& settings) const {
    const document::Outcome outcome = document_->settings(settings);
    if (report_) {
        report_(outcome);
    }
}

};  // namespace v3d::editor::control
