/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "MoyaDelegate.h"

#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <moya/libmoya/integrate/Picture.h>
#include <moya/libmoya/integrate/Pictures.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/integrate/Session.h>
#include <moya/libmoya/scene/Scene.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <mutex>
#include <stop_token>
#include <thread>
#include <utility>
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
    stopping_.store(true);
    worker_ = std::jthread();
}

/**
 **/
void MoyaDelegate::cancel() {
    std::unique_lock lock(mutex_);
    if (status_.state != State::Rendering) {
        return;
    }
    stopping_.store(true);
    woken_.wait(lock, [this] { return status_.state != State::Rendering; });
}

/**
 **/
void MoyaDelegate::resume() {
    const std::scoped_lock lock(mutex_);
    if (status_.state != State::Stopped) {
        return;
    }
    stopping_.store(false);
    resumed_ = true;
    status_.state = State::Rendering;
    woken_.notify_all();
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
void MoyaDelegate::show(const Showing& showing) {
    const std::scoped_lock lock(mutex_);
    if (showing == showing_) {
        return;
    }
    showing_ = showing;
    reshow_ = true;
    woken_.notify_all();
}

/**
 **/
boost::shared_ptr<v3d::image::Image> MoyaDelegate::picture(Channel channel) const {
    const std::scoped_lock lock(mutex_);
    const boost::shared_ptr<v3d::image::Image>& held = pictures_.at(static_cast<std::size_t>(channel));
    return held ? held : pictures_.at(static_cast<std::size_t>(Channel::Beauty));
}

/**
 **/
bool MoyaDelegate::translate(const scene::Scene& scene, const v3d::type::camera::Profile& from, const scene::RenderSettings& settings) {
    const scene::Evaluation evaluation = scene.evaluate();
    const RIBExportVisitor::Output output{ .settings = settings };
    return translator_.update(scene, evaluation, from, output);
}

/**
 **/
void MoyaDelegate::start(const scene::Scene& scene, const v3d::type::camera::Profile& from, const scene::RenderSettings& settings) {
    // the render running ends, and its session with it, so the next starts from nothing
    stopping_.store(true);
    worker_ = std::jthread();
    session_.reset();
    translate(scene, from, settings);
    stopping_.store(false);
    {
        const std::scoped_lock lock(mutex_);
        status_.state = State::Rendering;
        status_.done = 0.0F;
    }
    worker_ = std::jthread([this](const std::stop_token& stop) { run(stop); });
}

/**
 **/
bool MoyaDelegate::edit(const scene::Scene& scene, const v3d::type::camera::Profile& from, const scene::RenderSettings& settings) {
    bool failed = false;
    {
        const std::scoped_lock lock(mutex_);
        failed = status_.state == State::Failed;
    }
    // a render that failed may have left its session part way through a build
    if (failed || !worker_.joinable()) {
        start(scene, from, settings);
        return true;
    }
    if (!translate(scene, from, settings)) {
        return false;
    }
    const std::scoped_lock lock(mutex_);
    edits_++;
    stopping_.store(false);
    status_.state = State::Rendering;
    status_.done = 0.0F;
    // a session not yet made applies the commit when it first builds
    if (session_) {
        session_->edited();
    }
    woken_.notify_all();
    return true;
}

/**
 **/
void MoyaDelegate::run(const std::stop_token& stop) {
    const auto failed = [this](const std::exception& error) {
        // the render's thread has no caller to throw to, so a failure is reported in its status
        logger_->get()->error("the render failed - {}", error.what());
        const std::scoped_lock lock(mutex_);
        status_.state = State::Failed;
        woken_.notify_all();
    };
    // what the session's pictures show, as the render's thread last took it from show()
    Showing applied;
    v3d::moya::integrate::Session* session = nullptr;
    try {
        if (!device_) {
            device_.emplace();
        }
        v3d::moya::integrate::Checkpoints checkpoints;
        checkpoints.stop = [this](std::uint32_t) { return stopping_.load(); };
        const std::scoped_lock lock(mutex_);
        session = &session_.emplace(*device_, retained_, logger_, colours_, checkpoints);
        applied = showing_;
        reshow_ = false;
        session->show(picturesOf(applied));
    } catch (const std::exception& error) {
        failed(error);
        return;
    }

    while (!stop.stop_requested()) {
        std::uint64_t seen = 0;
        bool reshown = false;
        bool resumed = false;
        Showing showing;
        {
            const std::scoped_lock lock(mutex_);
            seen = edits_;
            reshown = std::exchange(reshow_, false);
            resumed = std::exchange(resumed_, false);
            showing = showing_;
        }
        bool more = false;
        try {
            // a change of what the pictures show remakes the latest picture, since a render that
            // has finished makes no more
            if (reshown) {
                const bool remade = showing.denoised != applied.denoised || showing.guides != applied.guides;
                applied = showing;
                session->show(picturesOf(applied));
                if (remade) {
                    session->again();
                }
            }
            if (resumed) {
                session->resume();
            }
            more = session->advance();
        } catch (const std::exception& error) {
            failed(error);
            return;
        }
        if (more) {
            continue;
        }

        // the film holds every sample, or the render was stopped. An edit committed since the
        // pass began is applied by the next
        std::unique_lock lock(mutex_);
        if (edits_ != seen) {
            continue;
        }
        if (stopping_.load()) {
            status_.state = State::Stopped;
        } else {
            status_.state = State::Finished;
            status_.done = 1.0F;
        }
        woken_.notify_all();
        woken_.wait(lock, stop, [this, seen] { return edits_ != seen || reshow_ || resumed_; });
    }
}

/**
 **/
v3d::moya::integrate::Pictures MoyaDelegate::picturesOf(const Showing& showing) {
    return { .interval = showing.interval,
        .denoised = showing.denoised,
        .guides = showing.guides,
        .shown = [this](const v3d::moya::integrate::Picture& picture) { shown(picture); } };
}

/**
 **/
void MoyaDelegate::shown(const v3d::moya::integrate::Picture& picture) {
    const auto image = [&picture](const std::vector<std::uint8_t>& bytes) {
        if (bytes.empty()) {
            return boost::shared_ptr<v3d::image::Image>();
        }
        const boost::shared_ptr<v3d::image::Image> made = boost::make_shared<v3d::image::Image>(glm::uvec2(picture.resolution), static_cast<std::uint8_t>(24));
        std::copy(bytes.begin(), bytes.end(), made->data());
        return made;
    };
    const std::array<boost::shared_ptr<v3d::image::Image>, 4> made{ image(picture.rgb), image(picture.denoised), image(picture.albedo), image(picture.normal) };
    const std::scoped_lock lock(mutex_);
    pictures_ = made;
    status_.pictures++;
    status_.samples = picture.samples;
    status_.total = picture.total;
    // a coarse picture adds nothing to the film
    if (picture.block == 1U && picture.total != 0U) {
        status_.done = std::min(static_cast<float>(picture.samples) / static_cast<float>(picture.total), 1.0F);
    }
}

};  // namespace v3d::editor::render
