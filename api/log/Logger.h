/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <spdlog/spdlog.h>

#include <memory>

namespace v3d::log {

    /**
     **/
    class Logger final {
     public:
            /**
             **/
            Logger();

            /**
             **/
            std::shared_ptr<spdlog::logger>& get();

     private:
         std::shared_ptr<spdlog::logger> logger_;
    };

};  // namespace v3d::log
