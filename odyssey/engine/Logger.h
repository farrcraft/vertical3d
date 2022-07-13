/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>

// Convenience macros
#define LOG_INFO(logger) BOOST_LOG_SEV(logger->get(), boost::log::trivial::info)
#define LOG_ERROR(logger) BOOST_LOG_SEV(logger->get(), boost::log::trivial::error)

namespace odyssey::engine {

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

};