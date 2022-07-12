/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>

// Convenience macros
#define LOG_DEBUG(logger) BOOST_LOG_SEV(logger->get(), boost::log::trivial::debug)
#define LOG_INFO(logger) BOOST_LOG_SEV(logger->get(), boost::log::trivial::info)
#define LOG_ERROR(logger) BOOST_LOG_SEV(logger->get(), boost::log::trivial::error)

namespace v3d::core {

    /**
     **/
    class Logger final {
     public:
            /**
             **/
            Logger();

            /**
             **/
            boost::log::sources::severity_logger< boost::log::trivial::severity_level >& get();

     private:
            boost::log::sources::severity_logger< boost::log::trivial::severity_level > logger_;
    };

};  // namespace v3d::core
