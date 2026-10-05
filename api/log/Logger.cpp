/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Logger.h"

#include <spdlog/sinks/basic_file_sink.h>

#include <memory>
#include <string>

namespace v3d::log {

namespace {

const char* const NAME = "v3d-logger";

std::string& destination() {
    static std::string path = "v3d.log";
    return path;
}

std::shared_ptr<spdlog::logger> create() {
    std::shared_ptr<spdlog::logger> logger = spdlog::basic_logger_mt(NAME, destination());
    logger->set_level(spdlog::level::debug);
    // a graphics app that goes wrong tends to stop responding rather than return from
    // main, and a buffered sink loses the lines that say what it was doing. Nothing in the
    // tree logs at info per frame, so flushing from info up costs a few writes at startup
    logger->flush_on(spdlog::level::info);
    return logger;
}

};  // namespace

/**
 **/
void Logger::open(const std::string& path) {
    destination() = path;
    // registering a name twice throws, so a log already in use is let go of first
    if (spdlog::get(NAME)) {
        spdlog::drop(NAME);
    }
    create();
}

/**
 **/
Logger::Logger() {
    logger_ = spdlog::get(NAME);
    if (!logger_) {
        logger_ = create();
    }
}

/**
 **/
std::shared_ptr<spdlog::logger> Logger::get() const {
    return logger_;
}

};  // namespace v3d::log
