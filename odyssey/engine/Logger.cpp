/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Logger.h"

#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/locale.hpp>

using namespace odyssey::engine;

/**
 **/
Logger::Logger() {
	// Setup logging
	auto sink = boost::log::add_file_log(
		boost::log::keywords::file_name = "odyssey_%N.log",                                        /*< file name pattern >*/
		boost::log::keywords::rotation_size = 10 * 1024 * 1024,                                   /*< rotate files every 10 MiB... >*/
		boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0), /*< ...or at midnight >*/
		boost::log::keywords::format = "[%TimeStamp%]: %Message%"                                 /*< log record format >*/
	);
	// Hard coding - log info or higher
	boost::log::core::get()->set_filter(
		boost::log::trivial::severity >= boost::log::trivial::info
	);

	std::locale loc = boost::locale::generator()("en_US.UTF-8");
	sink->imbue(loc);

	boost::log::add_common_attributes();
}

/**
 **/
boost::log::sources::severity_logger< boost::log::trivial::severity_level > &Logger::get() {
	return logger_;
}