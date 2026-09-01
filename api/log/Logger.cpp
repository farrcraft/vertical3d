/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Logger.h"

#include <spdlog/sinks/basic_file_sink.h>

namespace v3d::log {

    /**
     **/
    Logger::Logger() {
        // spdlog's registry is global and throws on a second registration under the same
        // name, so a second Logger takes over the one already registered rather than
        // bringing the process down. Apps really do build two - tetris constructs one in its
        // Controller before Engine::initialize constructs its own.
        logger_ = spdlog::get("v3d-logger");
        if (logger_) {
            return;
        }
        logger_ = spdlog::basic_logger_mt("v3d-logger", "v3d.log");
        logger_->set_level(spdlog::level::debug);
    }

    /**
     **/
    std::shared_ptr<spdlog::logger>& Logger::get() {
        return logger_;
    }

};  // namespace v3d::log
