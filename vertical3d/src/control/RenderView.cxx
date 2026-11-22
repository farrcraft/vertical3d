/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "RenderView.h"

#include <api/asset/Json.h>
#include <api/image/Image.h>
#include <api/type/camera/Profile.h>
#include <api/ui/Image.h>
#include <api/ui/Immediate.h>
#include <vertical3d/src/document/Outcome.h>
#include <vertical3d/src/render/Batch.h>
#include <vertical3d/src/render/Delegate.h>
#include <vertical3d/src/scene/Scene.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/json.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

namespace {

/** Each channel, by its value, with its name in an editor's state and its label in the view. **/
constexpr std::array<std::pair<std::string_view, std::string_view>, 4> CHANNELS{ {
    { "beauty", "Beauty" },
    { "denoised", "Denoised" },
    { "albedo", "Albedo" },
    { "normal", "Normal" },
} };

/** How far each tick moves the average seconds a tick takes toward its own. **/
constexpr double FRAME_WEIGHT = 0.1;

/** How far the average may drift from the interval last asked for before it is asked again. **/
constexpr double DRIFT = 0.5;

/**
 * @return "n of m samples", or nothing when a render takes no samples
 **/
std::string samplesOf(const render::Delegate::Status& status) {
    if (status.total == 0U) {
        return "";
    }
    if (status.samples >= status.total) {
        return " - " + std::to_string(status.total) + " samples";
    }
    return " - " + std::to_string(status.samples) + " of " + std::to_string(status.total) + " samples";
}

};  // namespace

RenderView::RenderView(const boost::shared_ptr<render::Delegate>& delegate, const boost::shared_ptr<render::Batch>& batch, const Upload& upload) :
    delegate_(delegate),
    batch_(batch),
    upload_(upload),
    state_("Nothing rendered yet"),
    frame_(0.0) {
}

void RenderView::render(const scene::Scene& scene, const v3d::type::camera::Profile& from) {
    delegate_->render(scene, from, scene.renderSettings());
    sync();
}

void RenderView::stop() {
    delegate_->cancel();
    batch_->cancel();
}

document::Outcome RenderView::batch(const scene::Scene* scene, const v3d::type::camera::Profile& view, const boost::filesystem::path& into) {
    const std::vector<render::Batch::Job> jobs = batch_->start(scene, view, scene->renderSettings(), into);
    return { .done = !jobs.empty(), .message = "rendering " + std::to_string(jobs.size()) + " frame(s) into " + into.string() };
}

render::Batch::Status RenderView::batchStatus() const {
    return batch_->status();
}

void RenderView::tick(const scene::Scene& scene, const v3d::type::camera::Profile& from, const std::vector<Channel>& shown, std::chrono::milliseconds since) {
    shown_ = shown;
    const double seconds = std::chrono::duration<double>(since).count();
    frame_ = frame_ <= 0.0 ? seconds : frame_ + ((seconds - frame_) * FRAME_WEIGHT);
    if (!shown.empty()) {
        render::Delegate::Showing showing{ .interval = std::max(LEAST_INTERVAL, frame_),
            .denoised = std::ranges::find(shown, Channel::Denoised) != shown.end(),
            .guides = std::ranges::find(shown, Channel::Albedo) != shown.end() || std::ranges::find(shown, Channel::Normal) != shown.end() };
        // an interval that drifts a little is not asked for again
        if (showing_ && std::abs(showing.interval - showing_->interval) <= showing_->interval * DRIFT) {
            showing.interval = showing_->interval;
        }
        if (showing != showing_) {
            showing_ = showing;
            delegate_->show(showing);
        }
        delegate_->update(scene, from, scene.renderSettings());
    }
    sync();
}

RenderView::Asked RenderView::draw(v3d::ui::Immediate* ui, std::size_t editor, const glm::vec4& region, Channel* channel) {
    const std::string id = "render view " + std::to_string(editor);
    ui->region(id, { .position = glm::vec2(region.x, region.y), .size = glm::vec2(region.z, region.w) }, 1.0F);
    ui->pushId(id);
    Asked asked = Asked::Nothing;
    if (ui->button("Render")) {
        asked = Asked::Render;
    }
    ui->sameLine();
    if (ui->button("Stop")) {
        asked = Asked::Stop;
    }
    ui->sameLine();
    std::vector<std::string> labels;
    labels.reserve(CHANNELS.size());
    for (const auto& [named, label] : CHANNELS) {
        labels.emplace_back(label);
    }
    int chosen = static_cast<int>(*channel);
    ui->nextItemWidth(CHANNEL_WIDTH);
    if (ui->combo("Channel", &chosen, labels) && chosen >= 0 && static_cast<std::size_t>(chosen) < CHANNELS.size()) {
        *channel = static_cast<Channel>(chosen);
    }
    ui->sameLine();
    ui->textDisabled(state_);
    const Shown& held = pictures_.at(static_cast<std::size_t>(*channel));
    const glm::vec2 room(ui->rowWidth(), ui->roomBelow());
    const glm::vec2 shown = fit(held.size, room);
    if (held.picture.valid() && shown.x > 0.0F && shown.y > 0.0F) {
        ui->image(held.picture, shown);
    }
    ui->popId();
    ui->endRegion();
    return asked;
}

const std::string& RenderView::state() const noexcept {
    return state_;
}

void RenderView::release() {
    delegate_->cancel();
    batch_->cancel();
}

std::string_view RenderView::name(Channel channel) {
    return CHANNELS.at(static_cast<std::size_t>(channel)).first;
}

RenderView::Channel RenderView::channelOf(const boost::json::object& state) {
    const std::optional<std::string> named = v3d::asset::readString(state, CHANNEL);
    for (std::size_t at = 0; named && at < CHANNELS.size(); at++) {
        if (CHANNELS.at(at).first == *named) {
            return static_cast<Channel>(at);
        }
    }
    return Channel::Beauty;
}

glm::vec2 RenderView::fit(const glm::uvec2& picture, const glm::vec2& room) {
    if (picture.x == 0 || picture.y == 0 || room.x <= 0.0F || room.y <= 0.0F) {
        return glm::vec2(0.0F);
    }
    const glm::vec2 size(picture);
    const float scale = std::min(room.x / size.x, room.y / size.y);
    return size * scale;
}

void RenderView::sync() {
    const render::Delegate::Status status = delegate_->status();
    const int done = static_cast<int>(status.done * 100.0F);  // checked: done is between 0 and 1
    switch (status.state) {
        case render::Delegate::State::Idle:
            state_ = "Nothing rendered yet";
            break;
        case render::Delegate::State::Rendering:
            state_ = "Rendering - " + std::to_string(done) + "%" + samplesOf(status);
            break;
        case render::Delegate::State::Finished:
            state_ = "Rendered" + samplesOf(status);
            break;
        case render::Delegate::State::Stopped:
            state_ = "Stopped" + samplesOf(status);
            break;
        case render::Delegate::State::Failed:
            state_ = "The render failed - the log says why";
            break;
    }
    // a channel is taken up when a new picture comes, or when a view first shows it
    if (status.pictures == 0U || !upload_) {
        return;
    }
    for (const Channel channel : shown_) {
        Shown& held = pictures_.at(static_cast<std::size_t>(channel));
        if (held.taken == status.pictures) {
            continue;
        }
        held.taken = status.pictures;
        const boost::shared_ptr<v3d::image::Image> image = delegate_->picture(channel);
        if (image) {
            held.picture = v3d::ui::Image(upload_(image));
            held.size = glm::uvec2(image->width(), image->height());
        }
    }
}

};  // namespace v3d::editor::control
