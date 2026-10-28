/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/rib/Reader.h>
#include <moya/libmoya/Planes.h>
#include <moya/libmoya/RIBHandler.h>
#include <spdlog/sinks/ringbuffer_sink.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace v3d::moya::handling {

/*
 * What the cases that read RIB through moya::RIBHandler share: reading a source into a handler,
 * counting what a frame covered, and hearing what the context logged.
 */

inline bool read(const std::string & source, v3d::moya::RIBHandler * handler) {
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());
    std::istringstream stream(source);
    return reader.read(stream, handler);
}

/**
 * The number of pixels whose red is more than half.
 **/
inline unsigned int coverage(const v3d::render::offline::FrameBuffer & planes) {
    unsigned int covered = 0;
    for (unsigned int row = 0; row < planes.height(); row++) {
        for (unsigned int column = 0; column < planes.width(); column++) {
            if (planes.value(v3d::moya::Planes::RED, column, row) > 0.5F) {
                covered++;
            }
        }
    }
    return covered;
}

/**
 * Collects the lines a logger writes while this object exists. It adds a ring buffer sink to
 * the logger and removes the sink when destroyed. A case reads the log output through it,
 * without reading the log file.
 **/
class Heard {
 public:
    explicit Heard(std::shared_ptr<spdlog::logger> logger) :
        logger_(std::move(logger)),
        sink_(std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(64)) {
        logger_->sinks().push_back(sink_);
    }

    ~Heard() {
        std::vector<spdlog::sink_ptr> & sinks = logger_->sinks();
        sinks.erase(std::remove(sinks.begin(), sinks.end(), sink_), sinks.end());
    }

    Heard(const Heard &) = delete;
    Heard & operator=(const Heard &) = delete;

    /**
     * Whether a line logged so far holds the text.
     **/
    bool said(const std::string & text) const {
        const std::vector<std::string> lines = sink_->last_formatted();
        return std::any_of(lines.begin(), lines.end(), [&text](const std::string & line) {
            return line.find(text) != std::string::npos;
        });
    }

 private:
    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> sink_;
};

/**
 * A quad 1.8 units across facing the camera, at one sample a pixel under a one pixel box, with
 * the grid size given. At 64 by 48 the quad is 43 pixels across. A grid of 16 is 4 a side, so
 * the quad is split four times over, into 256 pieces under 3 pixels across. A grid of 4096 is
 * 64 a side, and dices it whole. Both make the same lattice of micropolygons, 64 a side, so
 * the two pictures agree wherever a split keeps what its vertices carried. The quad is wound
 * clockwise as the camera sees it, from its lower left corner, so its geometric normal faces
 * the camera.
 **/
inline boost::shared_ptr<v3d::render::offline::FrameBuffer> quad(unsigned int grid, const std::string & shading,
    const std::string & varying) {
    v3d::moya::RIBHandler handler;
    BOOST_REQUIRE(read(
        "Option \"limits\" \"gridsize\" [" + std::to_string(grid) + "]\n"
        "Format 64 48 1\n"
        "PixelSamples 1 1\n"
        "PixelFilter \"box\" 1 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n" + shading +
        "Polygon \"P\" [-0.9 -0.9 5  -0.9 0.9 5  0.9 0.9 5  0.9 -0.9 5]\n" + varying +
        "WorldEnd\n", &handler));
    return handler.context().picture();
}

/**
 * Pixels inside the quad, spread over it.
 **/
inline constexpr unsigned int INSIDE[][2] = {
    { 14, 8 }, { 32, 8 }, { 50, 8 },
    { 14, 24 }, { 22, 24 }, { 32, 24 }, { 42, 24 },
    { 14, 40 }, { 42, 40 }, { 50, 40 }
};

};  // namespace v3d::moya::handling
