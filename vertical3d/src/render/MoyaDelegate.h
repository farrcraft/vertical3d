/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/integrate/Picture.h>
#include <moya/libmoya/integrate/Session.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>

#include "Delegate.h"
#include "SceneTranslator.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::render {

/**
 * Renders with moya's path tracer, linked into the editor, through one render session kept for
 * as long as the editor renders.
 *
 * **An edit reaches the running render.** update() brings moya's retained scene in step with the
 * editor's on the caller's thread, changing only what moved, and commits the change. The session
 * drops the pass in progress, applies the change, rebuilds only what it touched and samples the
 * edited scene from its first sample. A render that had finished or stopped starts again.
 *
 * **The session runs on a thread of the delegate's.** The thread runs one pass after another,
 * and waits for an edit once the film holds every sample. It owns the retained scene: it applies
 * the commits and is the only thread that reads the scene while it renders.
 *
 * **A picture follows each pass**, at most one every kInterval seconds but for the first of each
 * edit and the last, as the session makes them. The first pictures after an edit are coarse. A
 * stop drops the pass in progress, and the latest picture then holds every sample before it.
 *
 * **The picture is the beauty as a display shows it**, through the colour configuration's default
 * display and view, 8 bits a channel, as moya writes a PNG output. The picture a render ends on
 * and moya.exe's render of the scene's RIB export give the same bytes.
 **/
class MoyaDelegate final : public Delegate {
 public:
    /** The least seconds between two pictures, but for the first of each edit and the last. **/
    static constexpr double kInterval = 0.1;

    explicit MoyaDelegate(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Stops the render and waits for its thread.
     **/
    ~MoyaDelegate() override;

    /**
     * Ask the render to stop, and wait until it has, which is before its next tile.
     **/
    void cancel() override;

    Status status() const override;
    boost::shared_ptr<v3d::image::Image> picture() const override;

    /**
     * moya's retained scene, which may be read only while no render runs: before the first, or
     * once status() is no longer Rendering.
     **/
    const v3d::moya::scene::Scene& retained() const noexcept;

 protected:
    void start(const scene::Scene& scene, const v3d::type::camera::Profile& from, const scene::RenderSettings& settings) override;
    bool edit(const scene::Scene& scene, const v3d::type::camera::Profile& from, const scene::RenderSettings& settings) override;

 private:
    /**
     * Bring the retained scene in step with the editor's, and commit what changed.
     *
     * @return whether anything changed
     **/
    bool translate(const scene::Scene& scene, const v3d::type::camera::Profile& from, const scene::RenderSettings& settings);

    /**
     * Make the session and run its passes, on the render's thread, until the thread is stopped.
     **/
    void run(const std::stop_token& stop);

    /**
     * Take a picture from the session, on the render's thread.
     **/
    void shown(const v3d::moya::integrate::Picture& picture);

    boost::shared_ptr<v3d::log::Logger> logger_;

    // what the render's thread and the caller share
    mutable std::mutex mutex_;
    std::condition_variable_any woken_;
    Status status_;
    boost::shared_ptr<v3d::image::Image> picture_;
    // the edits committed, counted
    std::uint64_t edits_ = 0;
    // asked by the session before each pass and each tile
    std::atomic<bool> stopping_{ false };

    // edited and committed on the caller's thread, and applied and read by the render's
    v3d::moya::scene::Scene retained_{ v3d::moya::scene::kinds() };
    SceneTranslator translator_{ &retained_ };
    const v3d::moya::colour::Config colours_;

    // the device is made by the first render's thread and kept. The session is made by each
    // render's thread, under the mutex, and kept until the next render starts
    std::optional<v3d::moya::device::CpuDevice> device_;
    std::optional<v3d::moya::integrate::Session> session_;
    std::jthread worker_;
};

};  // namespace v3d::editor::render
