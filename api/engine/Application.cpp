/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Application.h"

#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace v3d::engine {

/**
 **/
std::string appPath(const char* executable) {
    const boost::filesystem::path directory =
        boost::filesystem::system_complete(boost::filesystem::path(executable)).remove_filename();
    return directory.string() + boost::filesystem::path("/").make_preferred().string();
}

};  // namespace v3d::engine
