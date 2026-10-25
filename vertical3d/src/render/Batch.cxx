/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Batch.h"

#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/RIBExportVisitor.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <cstddef>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "Process.h"
#include "Shot.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

namespace {

/**
 * A frame's file name: the camera's name and the frame, four digits wide so that the files
 * sort in frame order.
 **/
std::string frameName(const std::string& camera, int frame, const char* extension) {
    std::ostringstream name;
    name << camera << "." << std::setw(4) << std::setfill('0') << frame << extension;
    return name.str();
}

};  // namespace

/**
 **/
Batch::Batch(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::filesystem::path& program) :
    logger_(logger),
    program_(program) {
}

/**
 **/
Batch::~Batch() {
    cancel();
}

/**
 **/
std::vector<Batch::Job> Batch::start(Scene* scene, const v3d::type::camera::Profile& view, const RenderSettings& settings,
    const boost::filesystem::path& directory) {
    cancel();
    if (scene == nullptr) {
        return {};
    }

    // each camera is named by its document id, which no other camera of the document has
    std::vector<std::pair<std::string, boost::shared_ptr<Camera>>> sources;
    for (const boost::shared_ptr<Item>& item : scene->items()) {
        const boost::shared_ptr<Camera> camera = as<Camera>(item);
        if (camera) {
            sources.emplace_back("camera" + std::to_string(camera->documentId()), camera);
        }
    }
    if (sources.empty()) {
        sources.emplace_back("view", boost::shared_ptr<Camera>());
    }

    const Timeline timeline = scene->timeline();
    std::vector<Job> jobs;
    bool written = true;
    for (int frame = timeline.start; frame <= timeline.end && written; frame++) {
        scene->pose(static_cast<float>(frame));
        for (const auto& [name, camera] : sources) {
            const boost::filesystem::path folder = directory / name;
            boost::system::error_code error;
            boost::filesystem::create_directories(folder, error);
            const Job job{ .rib = folder / frameName(name, frame, ".rib"), .picture = folder / frameName(name, frame, ".png") };
            std::ofstream file(job.rib.string());
            if (!file.is_open()) {
                logger_->get()->error("could not write {}", job.rib.string());
                written = false;
                break;
            }
            // a RIB string is read with backslashes as escapes, so the picture's path is written
            // with forward slashes
            writeShot(&file, *scene, shotFrom(*scene, camera, view),
                RIBExportVisitor::Output{ .settings = settings, .file = job.picture.generic_string() });
            jobs.push_back(job);
        }
    }
    scene->pose(static_cast<float>(timeline.frame));
    if (!written) {
        return {};
    }

    {
        const std::scoped_lock lock(mutex_);
        status_ = Status{ .running = true, .total = jobs.size() };
    }
    worker_ = std::jthread([this, jobs](const std::stop_token& stop) { run(jobs, stop); });
    return jobs;
}

/**
 **/
void Batch::cancel() {
    if (worker_.joinable()) {
        worker_.request_stop();
        worker_.join();
    }
}

/**
 **/
Batch::Status Batch::status() const {
    const std::scoped_lock lock(mutex_);
    return status_;
}

/**
 **/
void Batch::run(const std::vector<Job>& jobs, const std::stop_token& stop) {
    for (const Job& job : jobs) {
        if (stop.stop_requested()) {
            break;
        }
        const int code = runProgram(program_, { "--file", job.rib.string(), "--silent" }, stop);
        const bool rendered = code == 0 && boost::filesystem::exists(job.picture);
        if (!rendered && !stop.stop_requested()) {
            logger_->get()->error("moya could not render {} - it ended with {}", job.rib.string(), code);
        }
        const std::scoped_lock lock(mutex_);
        status_.done++;
        status_.failed += rendered ? 0 : 1;
    }
    const std::scoped_lock lock(mutex_);
    status_.running = false;
}

};  // namespace v3d::editor
