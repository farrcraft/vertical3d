/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Image.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/json.hpp>

namespace v3d::editor {

/**
 * The editor's icons, as cells of one image, so every icon the ui draws shares a texture.
 *
 * The sheet is a document naming the image, how many cells a row of it holds, and the name of
 * each cell in order, row by row:
 *
 * ```json
 * { "image": "icons/icons.png", "columns": 5, "names": ["select", "move", "rotate"] }
 * ```
 *
 * The image is the cells and nothing else, so a cell's corners follow from its place. The ui
 * names an icon of the sheet as "icon:" and its name, as a button's icon or a menu item's.
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
     * Read a sheet. One that names no image, no columns or no names leaves the sheet as it was.
     *
     * @return whether it was understood
     **/
    bool load(const boost::json::object& sheet);

    /**
     * @return the image the cells are in
     **/
    const std::string& image() const noexcept;

    /**
     * @return the corners of the cell an image source names, or nothing when the source is not
     *         one of the sheet's icons
     **/
    std::optional<v3d::ui::Image::Corners> corners(std::string_view source) const;

 private:
    std::string image_;
    std::size_t columns_ = 0;
    std::vector<std::string> names_;
};

};  // namespace v3d::editor
