/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "WindowSize.h"

#include <api/asset/Json.h>

#include <optional>

namespace v3d::engine {

std::optional<glm::ivec2> windowSize(const boost::json::object& document) {
    const boost::json::object* window = v3d::asset::readObject(document, "window");
    if (window == nullptr) {
        return std::nullopt;
    }
    const std::optional<int> width = v3d::asset::readInt(*window, "width");
    const std::optional<int> height = v3d::asset::readInt(*window, "height");
    if (!width || !height) {
        return std::nullopt;
    }
    return glm::ivec2(*width, *height);
}

};  // namespace v3d::engine
