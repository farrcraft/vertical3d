/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <cstddef>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * Every frame of the timeline's range, rendered from each scene camera, by moya.exe.
 *
 * Each frame of each camera is two files: a RIB file and the picture moya.exe writes from it.
 * A scene with no camera is rendered from the view instead. The RIB files are written when the
 * batch starts, on the caller's thread, each from an evaluation of its frame, so a pending edit
 * is not rendered and the scene is not changed. A thread then runs moya.exe on each file in
 * turn, and a stop ends the one that is running and runs no more. The files stay where they
 * are, so a farm or another RenderMan renderer can render them instead.
 **/
class Batch final {
 public:
    /**
     * One frame from one camera: the RIB file and the picture it names.
     **/
    struct Job {
        boost::filesystem::path rib;
        boost::filesystem::path picture;
    };

    struct Status {
        bool running = false;
        std::size_t done = 0;
        std::size_t failed = 0;
        std::size_t total = 0;
    };

    /**
     * @param program the moya executable
     **/
    Batch(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::filesystem::path& program);

    /**
     * Stops a running batch and waits for its thread.
     **/
    ~Batch();

    Batch(const Batch&) = delete;
    Batch& operator=(const Batch&) = delete;

    /**
     * Write every frame's RIB file under a directory, one directory per camera, and start
     * rendering them. A batch already running is stopped first.
     *
     * @param view the camera to render from when the scene has none, and whose picture shape
     *        a scene camera takes
     * @return the jobs, or none when a file could not be written, in which case nothing runs
     **/
    std::vector<Job> start(const Scene* scene, const v3d::type::camera::Profile& view, const RenderSettings& settings,
        const boost::filesystem::path& directory);

    /**
     * Stop the batch: end the render that is running and start no other.
     **/
    void cancel();

    Status status() const;

 private:
    void run(const std::vector<Job>& jobs, const std::stop_token& stop);

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::filesystem::path program_;
    mutable std::mutex mutex_;
    Status status_;
    std::jthread worker_;
};

};  // namespace v3d::editor
