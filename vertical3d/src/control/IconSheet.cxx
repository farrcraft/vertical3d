/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "IconSheet.h"

#include <api/asset/Json.h>
#include <api/ui/Image.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/json.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

/**
 **/
bool IconSheet::load(const boost::json::object& sheet) {
    const std::optional<std::string> image = v3d::asset::readString(sheet, "image");
    const boost::json::value* columns = sheet.if_contains("columns");
    const boost::json::value* names = sheet.if_contains("names");
    if (!image || image->empty() || columns == nullptr || !columns->is_int64() || columns->as_int64() <= 0 || names == nullptr ||
        !names->is_array() || names->as_array().empty()) {
        return false;
    }
    std::vector<std::string> read;
    for (const boost::json::value& name : names->as_array()) {
        if (!name.is_string()) {
            return false;
        }
        read.emplace_back(name.as_string());
    }
    image_ = *image;
    columns_ = static_cast<std::size_t>(columns->as_int64());
    names_ = std::move(read);
    return true;
}

/**
 **/
const std::string& IconSheet::image() const noexcept {
    return image_;
}

/**
 **/
std::optional<v3d::ui::Image::Corners> IconSheet::corners(std::string_view source) const {
    if (!source.starts_with(PREFIX) || columns_ == 0) {
        return std::nullopt;
    }
    const auto found = std::ranges::find(names_, source.substr(PREFIX.size()));
    if (found == names_.end()) {
        return std::nullopt;
    }
    const auto index = static_cast<std::size_t>(found - names_.begin());
    const std::size_t rows = (names_.size() + columns_ - 1) / columns_;
    const glm::vec2 cell(1.0F / static_cast<float>(columns_), 1.0F / static_cast<float>(rows));
    const std::size_t column = index % columns_;
    const std::size_t row = index / columns_;
    const glm::vec2 min(static_cast<float>(column) * cell.x, static_cast<float>(row) * cell.y);
    return v3d::ui::Image::Corners{ .min = min, .max = min + cell };
}

};  // namespace v3d::editor
