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
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>

#include "Delegate.h"
#include "SceneTranslator.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::render {

/**
 * Renders with moya's path tracer, linked into the editor, on a thread of its own.
 *
 * **The retained scene is changed only between renders.** A render starts by bringing moya's
 * retained scene in step with the editor's on the caller's thread, changing only what moved. The
 * render's thread then reads it until the render ends. A render already running is cancelled
 * first, so the scene is never changed while a render reads it. The user can go on editing, and
 * update() starts the render again.
 *
 * **Progress is counted in passes.** Before each pass the render reports the passes done over the
 * passes its samples take, and stops there when it is cancelled. A render whose pixels all stop
 * early ends at 1 all the same.
 *
 * **The picture is the beauty as a display shows it**, through the colour configuration's default
 * display and view, 8 bits a channel, as moya writes a PNG output. A render of a scene and
 * moya.exe's render of its RIB export give the same bytes.
 **/
class MoyaDelegate final : public Delegate {
 public:
    explicit MoyaDelegate(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Cancels a running render and waits for its thread.
     **/
    ~MoyaDelegate() override;

    /**
     * Ask the render to stop, and wait until it has, which is before its next pass.
     **/
    void cancel() override;

    Status status() const override;
    boost::shared_ptr<v3d::image::Image> picture() const override;

    /** moya's retained scene, as the last render read it. **/
    const v3d::moya::scene::Scene& retained() const noexcept;

 protected:
    void start(const scene::Scene& scene, const v3d::type::camera::Profile& from, const scene::RenderSettings& settings) override;

 private:
    /**
     * Render the retained scene on the render's thread.
     **/
    void run(const std::stop_token& stop);

    boost::shared_ptr<v3d::log::Logger> logger_;
    // the status and the picture, which the render's thread writes and the caller reads
    mutable std::mutex mutex_;
    Status status_;
    boost::shared_ptr<v3d::image::Image> picture_;
    std::jthread worker_;

    // read and written on the caller's thread while no render runs, and read by the render's
    // thread while one does
    v3d::moya::scene::Scene retained_{ v3d::moya::scene::kinds() };
    SceneTranslator translator_{ &retained_ };
    const v3d::moya::colour::Config colours_;

    // made by the first render, on its thread, and used by one render at a time
    std::optional<v3d::moya::device::CpuDevice> device_;
};

};  // namespace v3d::editor::render
