/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/offline/rib/Recorder.h>
#include <api/type/camera/Profile.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <mutex>
#include <stop_token>
#include <thread>

#include "Delegate.h"
#include "SceneTranslator.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::render {

/**
 * Renders with moya, linked into the editor, on a thread of its own.
 *
 * The scene's requests are recorded on the caller's thread when a render starts, so the render
 * reads nothing of the scene while it runs and the user can go on editing. The thread replays
 * them into a moya handler of its own, and the picture stays in that handler's framebuffer
 * rather than going through a file. moya reports its progress per bucket or per row, and stops
 * there when the render is cancelled.
 *
 * **It keeps moya's new scene in step.** Each render also brings the new core's retained scene
 * in step with the editor's, changing only what moved, and applies the batch on the caller's
 * thread. The new core does not render yet, so the picture still comes from the shot.
 **/
class MoyaDelegate final : public Delegate {
 public:
    explicit MoyaDelegate(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Cancels a running render and waits for its thread.
     **/
    ~MoyaDelegate() override;

    /**
     * Ask the render to stop, and wait until it has, which is at the end of the bucket or row
     * it is on.
     **/
    void cancel() override;

    Status status() const override;
    boost::shared_ptr<v3d::image::Image> picture() const override;

    /** moya's new scene, as the last render left it. **/
    const v3d::moya::scene::Scene& retained() const noexcept;

 protected:
    void start(const scene::Scene& scene, const v3d::type::camera::Profile& from, const scene::RenderSettings& settings) override;

 private:
    /**
     * Render one recorded shot on the render's thread.
     **/
    void run(const v3d::render::offline::rib::Recorder& shot, const std::stop_token& stop);

    boost::shared_ptr<v3d::log::Logger> logger_;
    // the status and the picture, which the render's thread writes and the caller reads
    mutable std::mutex mutex_;
    Status status_;
    boost::shared_ptr<v3d::image::Image> picture_;
    std::jthread worker_;

    // read and written on the caller's thread alone
    v3d::moya::scene::Scene retained_{ v3d::moya::scene::kinds() };
    SceneTranslator translator_{ &retained_ };
};

};  // namespace v3d::editor::render
