/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace v3d::editor::control {

/**
 * @return text with every letter in lower case
 **/
std::string lower(std::string_view text);

/**
 * The words of a query that filters a list, in lower case. Words are separated by any run of
 * white space.
 *
 * @return the words, in the order they are written
 **/
std::vector<std::string> words(std::string_view query);

};  // namespace v3d::editor::control
