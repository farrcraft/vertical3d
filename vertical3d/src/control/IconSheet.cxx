/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "IconSheet.h"

#include <api/asset/Json.h>
#include <api/image/Image.h>
#include <api/image/reader/Svg.h>
#include <api/log/Logger.h>
#include <api/ui/Image.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor::control {

namespace {

/**
 * The bytes in a pixel of the sheet: four channels of eight bits.
 **/
constexpr std::size_t PIXEL = 4;

};  // namespace

/**
 **/
bool IconSheet::load(const boost::json::object& sheet) {
    const std::optional<std::string> directory = v3d::asset::readString(sheet, "directory");
    const boost::json::value* names = sheet.if_contains("names");
    if (!directory || directory->empty() || names == nullptr || !names->is_array() || names->as_array().empty()) {
        return false;
    }
    std::vector<std::string> read;
    for (const boost::json::value& name : names->as_array()) {
        if (!name.is_string() || name.as_string().empty()) {
            return false;
        }
        read.emplace_back(name.as_string());
    }
    directory_ = *directory;
    names_ = std::move(read);
    return true;
}

/**
 **/
std::string IconSheet::file(const std::string& name) const {
    return directory_ + "/" + name + ".svg";
}

/**
 **/
const std::vector<std::string>& IconSheet::names() const noexcept {
    return names_;
}

/**
 **/
std::size_t IconSheet::columns() const noexcept {
    return static_cast<std::size_t>(std::ceil(std::sqrt(static_cast<double>(names_.size()))));  // checked: the root of a count is finite and no larger
}

/**
 **/
std::size_t IconSheet::rows() const noexcept {
    const std::size_t across = columns();
    return across == 0 ? 0 : (names_.size() + across - 1) / across;
}

/**
 **/
std::optional<v3d::ui::Image::Corners> IconSheet::corners(std::string_view source) const {
    if (!source.starts_with(PREFIX) || names_.empty()) {
        return std::nullopt;
    }
    const auto found = std::ranges::find(names_, source.substr(PREFIX.size()));
    if (found == names_.end()) {
        return std::nullopt;
    }
    const auto index = static_cast<std::size_t>(found - names_.begin());
    const std::size_t across = columns();
    const glm::vec2 cell(1.0F / static_cast<float>(across), 1.0F / static_cast<float>(rows()));
    const std::size_t column = index % across;
    const std::size_t row = index / across;
    const glm::vec2 min(static_cast<float>(column) * cell.x, static_cast<float>(row) * cell.y);
    return v3d::ui::Image::Corners{ .min = min, .max = min + cell };
}

/**
 **/
boost::shared_ptr<v3d::image::Image> IconSheet::draw(unsigned int side, const Read& read, const boost::shared_ptr<v3d::log::Logger>& logger) const {
    if (names_.empty() || side == 0) {
        return nullptr;
    }
    const std::size_t across = columns();
    const glm::uvec2 size(static_cast<unsigned int>(across) * side, static_cast<unsigned int>(rows()) * side);
    boost::shared_ptr<v3d::image::Image> sheet = boost::make_shared<v3d::image::Image>(size, static_cast<std::uint8_t>(PIXEL * 8));
    v3d::image::reader::Svg svg(logger);
    for (std::size_t index = 0; index < names_.size(); index++) {
        const std::optional<std::string> text = read ? read(file(names_[index])) : std::nullopt;
        const boost::shared_ptr<v3d::image::Image> icon =
            text ? svg.read(reinterpret_cast<const unsigned char*>(text->data()), text->size(), glm::uvec2(side, side)) : nullptr;
        if (!icon) {
            logger->get()->error("The icon {} could not be drawn from {}", names_[index], file(names_[index]));
            continue;
        }
        // the cell's rows, copied into the sheet where the cell sits
        const std::size_t left = (index % across) * side;
        const std::size_t top = (index / across) * side;
        for (std::size_t row = 0; row < side; row++) {
            std::memcpy(sheet->data() + ((((top + row) * size.x) + left) * PIXEL), icon->data() + (row * side * PIXEL), side * PIXEL);
        }
    }
    return sheet;
}

};  // namespace v3d::editor::control
