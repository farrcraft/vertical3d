/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Json.h"

#include <string>

namespace v3d::asset {

std::optional<std::string> readString(const boost::json::object& object, std::string_view key) {
    const boost::json::value* member = object.if_contains(key);
    const boost::json::string* text = member == nullptr ? nullptr : member->if_string();
    if (text == nullptr) {
        return std::nullopt;
    }
    return std::string(text->data(), text->size());
}

std::optional<double> readNumber(const boost::json::object& object, std::string_view key) noexcept {
    const boost::json::value* member = object.if_contains(key);
    if (member == nullptr || !member->is_number()) {
        return std::nullopt;
    }
    boost::system::error_code error;
    const double number = member->to_number<double>(error);
    if (error) {
        return std::nullopt;
    }
    return number;
}

std::optional<bool> readBool(const boost::json::object& object, std::string_view key) noexcept {
    const boost::json::value* member = object.if_contains(key);
    if (member == nullptr || !member->is_bool()) {
        return std::nullopt;
    }
    return member->get_bool();
}

const boost::json::object* readObject(const boost::json::object& object, std::string_view key) noexcept {
    const boost::json::value* member = object.if_contains(key);
    return member == nullptr ? nullptr : member->if_object();
}

const boost::json::array* readArray(const boost::json::object& object, std::string_view key) noexcept {
    const boost::json::value* member = object.if_contains(key);
    return member == nullptr ? nullptr : member->if_array();
}

};  // namespace v3d::asset
