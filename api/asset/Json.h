/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <boost/json.hpp>

namespace v3d::asset {

/**
 * Checked reads of one member of a JSON object, by key.
 *
 * Each read returns nothing when the object has no member of that name, or when the member is
 * not of the type the read names. None of them throws for either case, where
 * boost::json::value_to and the as_ accessors throw. A caller that reports a missing member and
 * a member of the wrong type differently tests contains() first.
 *
 * The object and array reads return a pointer into the object instead of a copy, and a null
 * pointer in place of nothing. The pointer is valid for as long as the object is unchanged.
 **/

/**
 * @return the member's text, or nothing when it is absent or not a string
 **/
std::optional<std::string> readString(const boost::json::object& object, std::string_view key);

/**
 * A member stored as a signed integer, an unsigned integer or a double is a number, and is
 * converted to a double. An integer beyond 2^53 loses precision. The value is not tested for
 * being finite.
 *
 * @return the member's value, or nothing when it is absent or not a number
 **/
std::optional<double> readNumber(const boost::json::object& object, std::string_view key) noexcept;

/**
 * @return the member's value, or nothing when it is absent or not true or false
 **/
std::optional<bool> readBool(const boost::json::object& object, std::string_view key) noexcept;

/**
 * @return the member, or null when it is absent or not an object
 **/
const boost::json::object* readObject(const boost::json::object& object, std::string_view key) noexcept;

/**
 * @return the member, or null when it is absent or not an array
 **/
const boost::json::array* readArray(const boost::json::object& object, std::string_view key) noexcept;

};  // namespace v3d::asset
