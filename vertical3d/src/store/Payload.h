/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>

#include <boost/json.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 * @return whether a payload has exactly these keys. A payload that does not says which key is
 *         missing, or that it has one too many, in the problem
 **/
bool exactly(const boost::json::object& payload, std::initializer_list<const char*> keys, std::string* problem);

/**
 * Read a finite number, or an array of count of them, from a payload whose keys have been
 * checked.
 *
 * @return false when the value is not a number, or not an array of count finite numbers
 **/
bool numbers(const boost::json::value& value, std::size_t count, float* out);

/**
 * @return a payload's string, or nothing when the value is not one
 **/
std::string_view word(const boost::json::value& value);

/**
 * A vector as a payload holds it: an array of three numbers.
 **/
boost::json::array vector(const glm::vec3& value);

};  // namespace v3d::editor
