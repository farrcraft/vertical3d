/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

namespace v3d::render::offline::rib {

/**
 * The whitespace separated words of a declaration string, such as "uniform color Kd".
 **/
std::vector<std::string> words(const std::string & text);

};  // namespace v3d::render::offline::rib
