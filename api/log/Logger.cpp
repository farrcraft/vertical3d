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
        logger_ = spdlog::basic_logger_mt("v3d-logger", "v3d.log");
        logger_->set_level(spdlog::level::debug);
    }

    /**
     **/
    std::shared_ptr<spdlog::logger>& Logger::get() {
        return logger_;
    }

};  // namespace v3d::log
