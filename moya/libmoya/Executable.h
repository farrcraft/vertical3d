/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <boost/filesystem/path.hpp>

namespace v3d::moya {

/**
 * The directory of the running executable, where what moya loads at run time is installed beside
 * it: the CPU's libraries and MaterialX's standard libraries.
 *
 * @throws std::runtime_error when the operating system does not give the executable's path
 **/
boost::filesystem::path executableDirectory();

};  // namespace v3d::moya
