/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Payload.h"

#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>

#include <boost/json.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 **/
bool exactly(const boost::json::object& payload, std::initializer_list<const char*> keys, std::string* problem) {
    for (const char* key : keys) {
        if (!payload.contains(key)) {
            *problem = std::string("its payload has no ") + key;
            return false;
        }
    }
    if (payload.size() != keys.size()) {
        *problem = "its payload has a field its version does not define";
        return false;
    }
    return true;
}

/**
 **/
bool numbers(const boost::json::value& value, std::size_t count, float* out) {
    if (count == 1) {
        if (!value.is_number()) {
            return false;
        }
        out[0] = static_cast<float>(value.to_number<double>());
        return std::isfinite(out[0]);
    }
    if (!value.is_array() || value.as_array().size() != count) {
        return false;
    }
    for (std::size_t index = 0; index < count; index++) {
        if (!numbers(value.as_array()[index], 1, &out[index])) {
            return false;
        }
    }
    return true;
}

/**
 **/
std::string_view word(const boost::json::value& value) {
    return value.is_string() ? std::string_view(value.as_string()) : std::string_view();
}

/**
 **/
boost::json::array vector(const glm::vec3& value) {
    return boost::json::array{ value.x, value.y, value.z };
}

};  // namespace v3d::editor
