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

/**
 **/
bool Logger::open(const std::string& path) {
    // spdlog throws when a name is registered twice, so drop the current log first
    if (spdlog::get(NAME)) {
        spdlog::drop(NAME);
    }
    bool opened = false;
    create(path, &opened);
    return opened;
}

/**
 **/
Logger::Logger() {
    logger_ = spdlog::get(NAME);
    if (!logger_) {
        bool opened = false;
        logger_ = create(DEFAULT_PATH, &opened);
    }
}

/**
 **/
std::shared_ptr<spdlog::logger> Logger::get() const {
    return logger_;
}

std::shared_ptr<spdlog::logger> Logger::create(const std::string& path, bool* opened) {
    std::shared_ptr<spdlog::logger> logger;
    std::string why;
    try {
        logger = spdlog::basic_logger_mt(NAME, path);
    } catch (const spdlog::spdlog_ex& error) {
        why = error.what();
        logger = spdlog::stderr_logger_mt(NAME);
    }
    *opened = why.empty();
    if (!*opened) {
        logger->warn("the log cannot be written to {}, so it goes to stderr: {}", path, why);
    }
    logger->set_level(spdlog::level::debug);
    // flush at info and above. A graphics app that fails often hangs instead of returning
    // from main, and a buffered sink would lose its last lines. Nothing logs at info every
    // frame, so the flush costs only a few writes at startup
    logger->flush_on(spdlog::level::info);
    return logger;
}

};  // namespace v3d::log
