/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "RenderView.h"

#include <api/image/Image.h>
#include <api/type/camera/Profile.h>
#include <api/ui/Image.h>
#include <api/ui/Immediate.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/render/Batch.h>
#include <vertical3d/src/render/Delegate.h>
#include <vertical3d/src/scene/Scene.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

RenderView::RenderView(const boost::shared_ptr<render::Delegate>& delegate, const boost::shared_ptr<render::Batch>& batch, const Upload& upload) :
    delegate_(delegate),
    batch_(batch),
    upload_(upload),
    shownPictures_(0),
    shownState_(render::Delegate::State::Idle),
    shownDone_(-1),
    state_("Nothing rendered yet"),
    size_(0) {
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

void RenderView::tick(const scene::Scene& scene, const v3d::type::camera::Profile& from, bool shown) {
    if (shown) {
        delegate_->update(scene, from, scene.renderSettings());
    }
    sync();
}

RenderView::Asked RenderView::draw(v3d::ui::Immediate* ui, std::size_t editor, const glm::vec4& region) {
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
    ui->textDisabled(state_);
    const glm::vec2 room(ui->rowWidth(), ui->roomBelow());
    const glm::vec2 shown = fit(size_, room);
    if (picture_.valid() && shown.x > 0.0F && shown.y > 0.0F) {
        ui->image(picture_, shown);
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
    if (status.state != shownState_ || done != shownDone_) {
        shownState_ = status.state;
        shownDone_ = done;
        switch (status.state) {
            case render::Delegate::State::Idle:
                state_ = "Nothing rendered yet";
                break;
            case render::Delegate::State::Rendering:
                state_ = "Rendering - " + std::to_string(done) + "%";
                break;
            case render::Delegate::State::Finished:
                state_ = "Rendered";
                break;
            case render::Delegate::State::Stopped:
                state_ = "Stopped";
                break;
            case render::Delegate::State::Failed:
                state_ = "The render failed - the log says why";
                break;
        }
    }
    if (status.pictures != shownPictures_) {
        shownPictures_ = status.pictures;
        const boost::shared_ptr<v3d::image::Image> image = delegate_->picture();
        if (image && upload_) {
            picture_ = v3d::ui::Image(upload_(image));
            size_ = glm::uvec2(image->width(), image->height());
        }
    }
}

};  // namespace v3d::editor::control
