/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include <memory>
#include <string>

namespace v3d::log {

namespace {

const char* const NAME = "v3d-logger";

std::string& destination() {
    static std::string path = "v3d.log";
    return path;
}

/**
 * The log, in the file destination() names, or on stderr when that file cannot be opened: a
 * directory that is not writable must not stop the app from starting.
 *
 * @param opened set to whether the file was opened
 **/
std::shared_ptr<spdlog::logger> create(bool* opened) {
    std::shared_ptr<spdlog::logger> logger;
    std::string why;
    try {
        logger = spdlog::basic_logger_mt(NAME, destination());
    } catch (const spdlog::spdlog_ex& error) {
        why = error.what();
        logger = spdlog::stderr_logger_mt(NAME);
    }
    *opened = why.empty();
    if (!*opened) {
        logger->warn("the log cannot be written to {}, so it goes to stderr: {}", destination(), why);
    }
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
bool Logger::open(const std::string& path) {
    destination() = path;
    // registering a name twice throws, so a log already in use is let go of first
    if (spdlog::get(NAME)) {
        spdlog::drop(NAME);
    }
    bool opened = false;
    create(&opened);
    return opened;
}

/**
 **/
Logger::Logger() {
    logger_ = spdlog::get(NAME);
    if (!logger_) {
        bool opened = false;
        logger_ = create(&opened);
    }
}

/**
 **/
std::shared_ptr<spdlog::logger> Logger::get() const {
    return logger_;
}

};  // namespace v3d::log
