/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "RenderWindow.h"

#include <api/image/Image.h>
#include <api/type/camera/Profile.h>
#include <api/ui/Image.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/render/Batch.h>
#include <vertical3d/src/render/Delegate.h>
#include <vertical3d/src/render/RenderPanels.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <optional>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::editor {

RenderWindow::RenderWindow(const boost::shared_ptr<RenderPanels>& panels, const boost::shared_ptr<Delegate>& delegate,
    const boost::shared_ptr<Batch>& batch, const Upload& upload) :
    panels_(panels),
    delegate_(delegate),
    batch_(batch),
    upload_(upload),
    shownPictures_(0),
    shownState_(Delegate::State::Idle),
    shownDone_(-1) {
}

void RenderWindow::show(const Scene& scene, const v3d::type::camera::Profile& from) {
    panels_->showView();
    delegate_->render(scene, from, scene.renderSettings());
    sync();
}

void RenderWindow::stop() {
    delegate_->cancel();
    batch_->cancel();
}

void RenderWindow::close() {
    delegate_->cancel();
    panels_->hideView();
}

void RenderWindow::openSettings(const RenderSettings& settings) {
    panels_->openSettings(settings);
}

void RenderWindow::closeSettings() {
    panels_->closeSettings();
}

Outcome RenderWindow::applySettings(Document* document) {
    const std::optional<RenderSettings> settings = panels_->settings();
    if (!settings) {
        return { .done = false, .message = "the render settings need a width and a height of 1 to " +
            std::to_string(RenderSettings::LARGEST_SIDE) + " and samples of 1 to " + std::to_string(RenderSettings::MOST_SAMPLES) };
    }
    panels_->closeSettings();
    return document->settings(*settings);
}

Outcome RenderWindow::batch(Scene* scene, const v3d::type::camera::Profile& view, const boost::filesystem::path& into) {
    const std::vector<Batch::Job> jobs = batch_->start(scene, view, scene->renderSettings(), into);
    return { .done = !jobs.empty(), .message = "rendering " + std::to_string(jobs.size()) + " frame(s) into " + into.string() };
}

Batch::Status RenderWindow::batchStatus() const {
    return batch_->status();
}

void RenderWindow::tick(const Scene& scene, const v3d::type::camera::Profile& from) {
    if (!panels_->viewVisible()) {
        return;
    }
    delegate_->update(scene, from, scene.renderSettings());
    sync();
}

bool RenderWindow::open() const {
    return panels_->viewVisible();
}

void RenderWindow::release() {
    delegate_->cancel();
    batch_->cancel();
}

void RenderWindow::sync() {
    const Delegate::Status status = delegate_->status();
    const int done = static_cast<int>(status.done * 100.0F);  // checked: done is between 0 and 1
    if (status.state != shownState_ || done != shownDone_) {
        shownState_ = status.state;
        shownDone_ = done;
        switch (status.state) {
            case Delegate::State::Idle:
                panels_->status("Nothing rendered yet");
                break;
            case Delegate::State::Rendering:
                panels_->status("Rendering - " + std::to_string(done) + "%");
                break;
            case Delegate::State::Finished:
                panels_->status("Rendered");
                break;
            case Delegate::State::Stopped:
                panels_->status("Stopped");
                break;
            case Delegate::State::Failed:
                panels_->status("The render failed - the log says why");
                break;
        }
    }
    if (status.pictures != shownPictures_) {
        shownPictures_ = status.pictures;
        const boost::shared_ptr<v3d::image::Image> image = delegate_->picture();
        if (image && upload_) {
            panels_->picture(v3d::ui::Image(upload_(image)), glm::uvec2(image->width(), image->height()));
        }
    }
}

};  // namespace v3d::editor
