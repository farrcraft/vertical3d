/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Arguments.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/json.hpp>

namespace v3d::editor::script {

namespace {

/**
 * @return whether a value is an array of strings only
 **/
bool strings(const boost::json::value& value) {
    return value.is_array() && std::all_of(value.as_array().begin(), value.as_array().end(),
        [](const boost::json::value& each) { return each.is_string(); });
}

/**
 * @return whether a value is an integer an int holds. A script's whole numbers arrive as
 *         integers, and a number written with a fraction or an exponent does not
 **/
bool whole(const boost::json::value& value) {
    if (value.is_int64()) {
        return value.as_int64() >= std::numeric_limits<int>::min() && value.as_int64() <= std::numeric_limits<int>::max();
    }
    return value.is_uint64() && value.as_uint64() <= static_cast<std::uint64_t>(std::numeric_limits<int>::max());
}

/**
 * @return what a value of a type is, as a refusal says it
 **/
std::string_view described(Type type) {
    switch (type) {
        case Type::Boolean:
            return "true or false";
        case Type::Number:
            return "a number";
        case Type::Integer:
            return "a whole number";
        case Type::Text:
            return "text";
        case Type::Item:
            return "an item's name";
        case Type::Items:
            return "a list of item names";
        case Type::Parts:
            return "a list of parts";
    }
    return "";
}

/**
 * @return whether a value is of a type
 **/
bool fits(const boost::json::value& value, Type type) {
    switch (type) {
        case Type::Boolean:
            return value.is_bool();
        case Type::Number:
            return value.is_number() && std::isfinite(value.to_number<double>());
        case Type::Integer:
            return whole(value);
        case Type::Text:
        case Type::Item:
            return value.is_string();
        case Type::Items:
        case Type::Parts:
            return strings(value);
    }
    return false;
}

};  // namespace

/**
 **/
Arguments::Arguments(boost::json::object given) :
    values_(std::move(given)) {
}

/**
 **/
std::optional<std::string> Arguments::check(const boost::json::object& given, const std::vector<Parameter>& parameters) {
    for (const auto& [key, value] : given) {
        const auto parameter = std::find_if(parameters.begin(), parameters.end(),
            [&key](const Parameter& each) { return each.name == std::string_view(key); });
        if (parameter == parameters.end()) {
            return "it takes no argument called " + std::string(key);
        }
        if (!fits(value, parameter->type)) {
            return std::string(key) + " is " + std::string(described(parameter->type));
        }
    }
    return std::nullopt;
}

/**
 **/
bool Arguments::boolean(std::string_view name, const std::function<bool()>& fill) {
    if (const boost::json::value* value = values_.if_contains(name)) {
        return value->as_bool();
    }
    const bool filled = fill();
    values_[name] = filled;
    return filled;
}

/**
 **/
double Arguments::number(std::string_view name, const std::function<double()>& fill) {
    if (const boost::json::value* value = values_.if_contains(name)) {
        return value->to_number<double>();
    }
    const double filled = fill();
    values_[name] = filled;
    return filled;
}

/**
 **/
int Arguments::integer(std::string_view name, const std::function<int()>& fill) {
    if (const boost::json::value* value = values_.if_contains(name)) {
        return value->to_number<int>();
    }
    const int filled = fill();
    values_[name] = filled;
    return filled;
}

/**
 **/
std::string Arguments::text(std::string_view name, const std::function<std::string()>& fill) {
    if (const boost::json::value* value = values_.if_contains(name)) {
        return std::string(value->as_string());
    }
    std::string filled = fill();
    values_[name] = boost::json::string(filled);
    return filled;
}

/**
 **/
std::vector<std::string> Arguments::names(std::string_view name, const std::function<std::vector<std::string>()>& fill) {
    if (const boost::json::value* value = values_.if_contains(name)) {
        std::vector<std::string> given;
        for (const boost::json::value& each : value->as_array()) {
            given.emplace_back(each.as_string());
        }
        return given;
    }
    std::vector<std::string> filled = fill();
    boost::json::array kept;
    for (const std::string& each : filled) {
        kept.emplace_back(boost::json::string(each));
    }
    values_[name] = std::move(kept);
    return filled;
}

/**
 **/
bool Arguments::has(std::string_view name) const {
    return values_.contains(name);
}

/**
 **/
const boost::json::object& Arguments::values() const noexcept {
    return values_;
}

};  // namespace v3d::editor::script
