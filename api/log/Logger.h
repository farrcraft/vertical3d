/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <string>

namespace v3d::log {

/**
 * The process's log.
 *
 * There is one, and every Logger is a handle on it: spdlog's registry is global, and a log
 * that split by who constructed it would be no use to read. So this is a global with an
 * object's spelling, and passing one around says where a class logs rather than giving it a
 * log of its own.
 **/
class Logger final {
 public:
        /**
         * Say where the log is written, before anything logs. engine::run() does this with
         * the directory the executable is in, so v3d.log lands beside it whatever directory
         * the app was started from. Without it the log is v3d.log in the working directory,
         * which is what a test or a tool run from its own directory wants.
         *
         * Called after something has logged, it moves the log there from then on; a handle
         * taken before still writes where it was taken.
         **/
        static void open(const std::string& path);

        /**
         **/
        Logger();

        /**
         **/
        std::shared_ptr<spdlog::logger> get() const;

 private:
     std::shared_ptr<spdlog::logger> logger_;
};

};  // namespace v3d::log
