/**
 * Voxel Engine
 *
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <cstdlib>
#include <string>

#include "Controller.h"


int main(int argc, char *argv[]) 
{
	// extract exe path from argv (needed for loading file assets with relative paths)
	boost::filesystem::path path(boost::filesystem::initial_path<boost::filesystem::path>());
    path = boost::filesystem::system_complete(boost::filesystem::path(argv[0])).remove_filename();

	boost::filesystem::path slash("/");
	std::string preferredSlash = slash.make_preferred().string();

	std::string appPath = path.string() + preferredSlash;

	// setup the logger
	std::string logfile = appPath + std::string("voxel.log");
	boost::log::add_file_log(
		boost::log::keywords::file_name = logfile,
		boost::log::keywords::format = "[%TimeStamp%]: %Severity% - %Message%"
	);

	// logging level set to debug
	boost::log::core::get()->set_filter(
		boost::log::trivial::severity >= boost::log::trivial::debug
	);

	boost::log::add_common_attributes();

	// seed the random number generator
	srand(static_cast<unsigned int>(time(NULL)));

	Controller controller(appPath);
	if (!controller.run())
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
