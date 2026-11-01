/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/ui/Image.h>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor::control {
/**
 * The editor's icons, each an SVG file drawn in white, drawn together into one image at the size
 * the ui needs them, so every icon the ui draws shares a texture. Whatever draws an icon tints it.
 *
 * The sheet is a document naming the directory the files are in, and each icon. An icon's file
 * is its name and ".svg":
 *
 * ```json
 * { "directory": "icons", "names": ["select", "move", "rotate"] }
 * ```
 *
 * The cells are laid out in the order of the names, row by row, in a grid as many columns wide as
 * the square root of the count, rounded up. A cell's corners follow from its place, so they are
 * the same whatever size the image is drawn at. The ui names an icon as "icon:" and its name, as
 * a button's icon or a menu item's.
 **/
class IconSheet final {
 public:
    /**
     * The name config.json gives the kind of the sheet's document.
     **/
    static constexpr std::string_view DOCUMENT = "icons";

    /**
     * What an image's source begins with when it is an icon of the sheet.
     **/
    static constexpr std::string_view PREFIX = "icon:";

    /**
     * What gives the text of a file, by its path within the data, or nothing when it cannot.
     **/
    using Read = std::function<std::optional<std::string>(const std::string& path)>;

    /**
     * Read a sheet. One that names no directory or no names leaves the sheet as it was.
     *
     * @return whether it was understood
     **/
    bool load(const boost::json::object& sheet);

    /**
     * @return the path within the data of an icon's file
     **/
    std::string file(const std::string& name) const;

    /**
     * @return the icons' names, in the order of their cells
     **/
    const std::vector<std::string>& names() const noexcept;

    /**
     * @return how many cells a row of the image holds
     **/
    std::size_t columns() const noexcept;

    /**
     * @return how many rows of cells the image holds
     **/
    std::size_t rows() const noexcept;

    /**
     * @return the corners of the cell an image source names, or nothing when the source is not
     *         one of the sheet's icons
     **/
    std::optional<v3d::ui::Image::Corners> corners(std::string_view source) const;

    /**
     * Draw every icon into one image, each scaled to fit a square cell the side given across. An
     * icon whose file cannot be read or drawn leaves its cell empty, and is logged.
     *
     * @return the image, or null for a sheet with no names or a side of none
     **/
    boost::shared_ptr<v3d::image::Image> draw(unsigned int side, const Read& read, const boost::shared_ptr<v3d::log::Logger>& logger) const;

 private:
    std::string directory_;
    std::vector<std::string> names_;
};

};  // namespace v3d::editor::control
