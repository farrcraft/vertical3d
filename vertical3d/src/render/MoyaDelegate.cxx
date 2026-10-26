/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MoyaDelegate.h"

#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/rib/Recorder.h>
#include <api/type/camera/Profile.h>
#include <moya/libmoya/Planes.h>
#include <moya/libmoya/RIBHandler.h>
#include <vertical3d/src/scene/RIBExportVisitor.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <exception>
#include <mutex>
#include <stop_token>
#include <thread>

#include "Delegate.h"
#include "Shot.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

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
boost::shared_ptr<v3d::image::Image> MoyaDelegate::picture() const {
    const std::scoped_lock lock(mutex_);
    return picture_;
}

/**
 **/
void MoyaDelegate::start(const Scene& scene, const v3d::type::camera::Profile& from, const RenderSettings& settings) {
    const boost::shared_ptr<v3d::render::offline::rib::Recorder> shot = boost::make_shared<v3d::render::offline::rib::Recorder>();
    shoot(shot.get(), scene, from, RIBExportVisitor::Output{ .settings = settings });
    {
        const std::scoped_lock lock(mutex_);
        status_.state = State::Rendering;
        status_.done = 0.0F;
    }
    worker_ = std::jthread([this, shot](const std::stop_token& stop) { run(*shot, stop); });
}

/**
 **/
void MoyaDelegate::run(const v3d::render::offline::rib::Recorder& shot, const std::stop_token& stop) {
    State finished = State::Failed;
    boost::shared_ptr<v3d::image::Image> image;
    try {
        v3d::moya::RIBHandler handler;
        handler.context().progress([this, &stop](float done) {
            const std::scoped_lock lock(mutex_);
            status_.done = done;
            return !stop.stop_requested();
        });
        shot.replay(&handler);
        if (handler.context().stopped()) {
            finished = State::Stopped;
        } else if (!handler.context().picture()) {
            logger_->get()->error("the render made no picture of the scene");
        } else {
            image = handler.context().picture()->image(v3d::moya::Planes::CHANNELS);
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

};  // namespace v3d::editor
