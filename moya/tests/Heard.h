/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace v3d::moya::handling {

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

};  // namespace v3d::moya::handling
