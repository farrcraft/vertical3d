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
 * There is one log, and every Logger is a handle on it, because spdlog's registry is global.
 * Passing a Logger to a class shows that the class logs; it does not give the class a log of
 * its own.
 **/
class Logger final {
 public:
        /**
         * Say where the log is written, before anything logs. engine::run() does this with
         * the directory the executable is in, so v3d.log lands beside it whatever directory
         * the app was started from. Without it the log is v3d.log in the working directory,
         * which suits a test or a tool run from its own directory.
         *
         * Called after something has logged, it moves the log there from then on; a handle
         * taken before still writes where it was taken.
         *
         * A path that cannot be opened, such as one in a directory that is not writable, does
         * not throw. The log goes to stderr instead, and its first line says why.
         *
         * @return whether the log is written to path
         **/
        static bool open(const std::string& path);

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
