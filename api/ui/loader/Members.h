/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>

namespace v3d::ui::loader {

/*
 * Reading the members of a ui document. The theme reader and the component reader both read
 * through these, so a document is held to one rule:
 *
 * - A required member, one the document cannot be read without, that is missing, holds the
 *   wrong type or names a word it does not take is logged as an error, and the document is
 *   refused.
 * - An entry of an array that holds the wrong type is refused the same way.
 * - An optional member that holds the wrong type, or names a word it does not take, is logged
 *   as a warning and read as absent, so the setting keeps its default.
 * - Nothing throws out of a read.
 */

/**
 * An optional string member: its text, or nothing when it is absent or not a string.
 **/
std::optional<std::string> optionalString(const v3d::log::Logger& logger, const boost::json::object& entry,
    std::string_view key);

/**
 * An optional number member a float can hold, or nothing.
 **/
std::optional<float> optionalNumber(const v3d::log::Logger& logger, const boost::json::object& entry,
    std::string_view key);

/**
 * An optional whole number member no smaller than a minimum, or nothing.
 **/
std::optional<int> optionalInt(const v3d::log::Logger& logger, const boost::json::object& entry,
    std::string_view key, int minimum);

/**
 * An optional true or false member, or the fallback.
 **/
bool optionalFlag(const v3d::log::Logger& logger, const boost::json::object& entry, std::string_view key,
    bool fallback);

/**
 * An optional array member, or null when it is absent or not an array.
 **/
const boost::json::array* optionalArray(const v3d::log::Logger& logger, const boost::json::object& entry,
    std::string_view key);

/**
 * A string member the document cannot do without, or nothing when it is missing or not a
 * string.
 *
 * @param what the thing that needs it, for the log
 **/
std::optional<std::string> requiredString(const v3d::log::Logger& logger, const boost::json::object& entry,
    std::string_view key, std::string_view what);

/**
 * An array member the document cannot do without, or null, logged as requiredString() is.
 **/
const boost::json::array* requiredArray(const v3d::log::Logger& logger, const boost::json::object& entry,
    std::string_view key, std::string_view what);

/**
 * An entry of an array that has to be an object, or null when it is not one.
 *
 * @param what what the array holds, for the log
 **/
const boost::json::object* requiredObject(const v3d::log::Logger& logger, const boost::json::value& entry,
    std::string_view what);

/**
 * Log an optional member that names a word it does not take.
 **/
void unknownWord(const v3d::log::Logger& logger, std::string_view key, std::string_view word);

/**
 * An optional member that names one of a set of words: the value the word stands for, or the
 * fallback when the member is absent, not a string, or a word not in the set.
 **/
template <typename T>
T optionalChoice(const v3d::log::Logger& logger, const boost::json::object& entry, std::string_view key,
    std::initializer_list<std::pair<std::string_view, T>> words, T fallback) {
    const std::optional<std::string> word = optionalString(logger, entry, key);
    if (!word) {
        return fallback;
    }
    for (const std::pair<std::string_view, T>& known : words) {
        if (known.first == *word) {
            return known.second;
        }
    }
    unknownWord(logger, key, *word);
    return fallback;
}

};  // namespace v3d::ui::loader
