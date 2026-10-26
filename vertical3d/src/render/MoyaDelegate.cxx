/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MoyaDelegate.h"

#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/rib/Reader.h>
#include <api/type/camera/Profile.h>
#include <moya/libmoya/Planes.h>
#include <moya/libmoya/RIBHandler.h>
#include <moya/libmoya/Renderer.h>
#include <vertical3d/src/scene/RIBExportVisitor.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <exception>
#include <mutex>
#include <sstream>
#include <stop_token>
#include <string>
#include <thread>

#include "Delegate.h"
#include "Shot.h"

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
    std::ostringstream rib;
    writeShot(&rib, scene, from, RIBExportVisitor::Output{ .settings = settings });
    {
        const std::scoped_lock lock(mutex_);
        status_.state = State::Rendering;
        status_.done = 0.0F;
    }
    worker_ = std::jthread([this, text = rib.str()](const std::stop_token& stop) { run(text, stop); });
}

/**
 **/
void MoyaDelegate::run(const std::string& rib, const std::stop_token& stop) {
    State finished = State::Failed;
    boost::shared_ptr<v3d::image::Image> image;
    try {
        v3d::moya::Renderer renderer;
        v3d::moya::RIBHandler handler(&renderer);
        handler.context().progress([this, &stop](float done) {
            const std::scoped_lock lock(mutex_);
            status_.done = done;
            return !stop.stop_requested();
        });
        v3d::render::offline::rib::Reader reader(logger_);
        std::istringstream stream(rib);
        const bool read = reader.read(stream, &handler);
        if (handler.context().stopped()) {
            finished = State::Stopped;
        } else if (!read || !handler.context().picture()) {
            logger_->get()->error("the render could not read the scene - {}", reader.error());
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
