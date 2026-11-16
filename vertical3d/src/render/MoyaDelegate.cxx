/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "MoyaDelegate.h"

#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/output/Output.h>
#include <moya/libmoya/output/Outputs.h>
#include <moya/libmoya/output/Png.h>
#include <moya/libmoya/scene/Scene.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include "Delegate.h"
#include "RIBExportVisitor.h"
#include "SceneTranslator.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor::render {

/**
 **/
MoyaDelegate::MoyaDelegate(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
}

/**
 **/
MoyaDelegate::~MoyaDelegate() {
    cancel();
}

/**
 **/
void MoyaDelegate::cancel() {
    if (worker_.joinable()) {
        worker_.request_stop();
        worker_.join();
    }
}

/**
 **/
Delegate::Status MoyaDelegate::status() const {
    const std::scoped_lock lock(mutex_);
    return status_;
}

/**
 **/
const v3d::moya::scene::Scene& MoyaDelegate::retained() const noexcept {
    return retained_;
}

/**
 **/
boost::shared_ptr<v3d::image::Image> MoyaDelegate::picture() const {
    const std::scoped_lock lock(mutex_);
    return picture_;
}

/**
 **/
void MoyaDelegate::start(const scene::Scene& scene, const v3d::type::camera::Profile& from, const scene::RenderSettings& settings) {
    const scene::Evaluation evaluation = scene.evaluate();
    const RIBExportVisitor::Output output{ .settings = settings };
    if (translator_.update(scene, evaluation, from, output)) {
        retained_.apply();
    }
    {
        const std::scoped_lock lock(mutex_);
        status_.state = State::Rendering;
        status_.done = 0.0F;
    }
    worker_ = std::jthread([this](const std::stop_token& stop) { run(stop); });
}

/**
 **/
void MoyaDelegate::run(const std::stop_token& stop) {
    State finished = State::Failed;
    boost::shared_ptr<v3d::image::Image> image;
    try {
        if (!device_) {
            device_.emplace();
        }
        const std::uint32_t passes = v3d::moya::integrate::passesOf(retained_);
        bool stopped = false;
        v3d::moya::integrate::Checkpoints checkpoints;
        checkpoints.stop = [this, &stop, &stopped, passes](std::uint32_t done) {
            {
                const std::scoped_lock lock(mutex_);
                status_.done = static_cast<float>(std::min(done, passes)) / static_cast<float>(passes);
            }
            stopped = stop.stop_requested();
            return stopped;
        };
        v3d::moya::film::Snapshot snapshot = v3d::moya::integrate::render(*device_, retained_, logger_, colours_, checkpoints);
        if (stopped) {
            finished = State::Stopped;
        } else {
            // composited over the first output's background, as moya writes that output
            const std::vector<v3d::moya::output::Output> outputs = v3d::moya::output::outputsOf(retained_);
            if (!outputs.empty() && outputs.front().background) {
                snapshot = v3d::moya::output::composited(snapshot, colours_.input(outputs.front().backgroundSpace).apply(*outputs.front().background));
            }
            const std::string display = colours_.defaultDisplay();
            const std::vector<std::uint8_t> shown = v3d::moya::output::displayed(snapshot, colours_.display(display, colours_.defaultView(display)));
            image = boost::make_shared<v3d::image::Image>(glm::uvec2(snapshot.resolution), static_cast<std::uint8_t>(24));
            std::copy(shown.begin(), shown.end(), image->data());
            finished = State::Finished;
        }
    } catch (const std::exception& error) {
        // the render's thread has no caller to throw to, so a failure is reported in its status
        logger_->get()->error("the render failed - {}", error.what());
    }

    const std::scoped_lock lock(mutex_);
    status_.state = finished;
    if (image) {
        picture_ = image;
        status_.done = 1.0F;
        status_.pictures++;
    }
}

};  // namespace v3d::editor::render
