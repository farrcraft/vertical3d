/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Members.h"

#include <api/asset/Json.h>
#include <api/log/Logger.h>

#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>

namespace v3d::ui::loader {

std::optional<std::string> optionalString(const v3d::log::Logger& logger, const boost::json::object& entry,
    std::string_view key) {
    if (!entry.contains(key)) {
        return std::nullopt;
    }
    std::optional<std::string> value = v3d::asset::readString(entry, key);
    if (!value) {
        logger.get()->warn("The ui member [{}] is not a string, so it is ignored", key);
    }
    return value;
}

std::optional<float> optionalNumber(const v3d::log::Logger& logger, const boost::json::object& entry,
    std::string_view key) {
    if (!entry.contains(key)) {
        return std::nullopt;
    }
    const std::optional<double> value = v3d::asset::readNumber(entry, key);
    if (!value || std::abs(*value) > static_cast<double>(std::numeric_limits<float>::max())) {
        logger.get()->warn("The ui member [{}] is not a number, so it is ignored", key);
        return std::nullopt;
    }
    return static_cast<float>(*value);
}

std::optional<int> optionalInt(const v3d::log::Logger& logger, const boost::json::object& entry,
    std::string_view key, int minimum) {
    if (!entry.contains(key)) {
        return std::nullopt;
    }
    const std::optional<int> value = v3d::asset::readInt(entry, key);
    if (!value || *value < minimum) {
        logger.get()->warn("The ui member [{}] is not a whole number from {} up, so it is ignored", key, minimum);
        return std::nullopt;
    }
    return value;
}

bool optionalFlag(const v3d::log::Logger& logger, const boost::json::object& entry, std::string_view key,
    bool fallback) {
    if (!entry.contains(key)) {
        return fallback;
    }
    const std::optional<bool> value = v3d::asset::readBool(entry, key);
    if (!value) {
        logger.get()->warn("The ui member [{}] is not true or false, so it is ignored", key);
        return fallback;
    }
    return *value;
}

const boost::json::array* optionalArray(const v3d::log::Logger& logger, const boost::json::object& entry,
    std::string_view key) {
    if (!entry.contains(key)) {
        return nullptr;
    }
    const boost::json::array* value = v3d::asset::readArray(entry, key);
    if (value == nullptr) {
        logger.get()->warn("The ui member [{}] is not an array, so it is ignored", key);
    }
    return value;
}

std::optional<std::string> requiredString(const v3d::log::Logger& logger, const boost::json::object& entry,
    std::string_view key, std::string_view what) {
    std::optional<std::string> value = v3d::asset::readString(entry, key);
    if (!value) {
        logger.get()->error("{} needs a string [{}]", what, key);
    }
    return value;
}

const boost::json::array* requiredArray(const v3d::log::Logger& logger, const boost::json::object& entry,
    std::string_view key, std::string_view what) {
    const boost::json::array* value = v3d::asset::readArray(entry, key);
    if (value == nullptr) {
        logger.get()->error("{} needs an array [{}]", what, key);
    }
    return value;
}

const boost::json::object* requiredObject(const v3d::log::Logger& logger, const boost::json::value& entry,
    std::string_view what) {
    if (!entry.is_object()) {
        logger.get()->error("{} has an entry that is not an object", what);
        return nullptr;
    }
    return &entry.as_object();
}

void unknownWord(const v3d::log::Logger& logger, std::string_view key, std::string_view word) {
    logger.get()->warn("The ui member [{}] has no value [{}], so it is ignored", key, word);
}

};  // namespace v3d::ui::loader
