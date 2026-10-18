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
 * There is one log, because spdlog's registry is global. Every Logger is a handle on it.
 * Passing a Logger to a class shows that the class logs. It does not give the class a log of
 * its own.
 **/
class Logger final {
 public:
        /**
         * Sets the file the log is written to. Call it before anything logs. engine::run()
         * passes v3d.log in the executable's directory, so the log is beside the executable
         * whatever the working directory. Without a call, the log is v3d.log in the working
         * directory.
         *
         * Called after something has logged, it moves the log for every Logger constructed
         * afterwards. A Logger constructed before the call still writes to the old file.
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
