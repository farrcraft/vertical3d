/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <stop_token>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>

namespace v3d::editor {

/**
 * Run a program and wait for it to end, ending it early if a stop is asked for meanwhile.
 *
 * The program gets no console window of its own, and its output goes wherever the editor's does.
 * The stop is checked several times a second, so a long render ends soon after it is asked to.
 *
 * @param arguments each passed as one argument, however many spaces it holds
 * @return the program's exit code, or -1 when it could not be started or was ended early
 **/
int runProgram(const boost::filesystem::path& program, const std::vector<std::string>& arguments,
    const std::stop_token& stop);

};  // namespace v3d::editor
