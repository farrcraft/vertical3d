/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Picture.h"

#include "TileCoord.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace v3d::grid {

Picture fromPicture(const std::vector<std::string>& rows, const std::map<char, Terrain>& legend, float tileSize) {
    Picture picture;
    if (rows.empty()) {
        picture.error = "the picture has no rows";
        return picture;
    }
    const std::size_t width = rows.front().size();
    if (width == 0) {
        picture.error = "the picture's first row is empty";
        return picture;
    }
    for (std::size_t y = 0; y < rows.size(); y++) {
        if (rows[y].size() != width) {
            picture.error = "row " + std::to_string(y) + " is " + std::to_string(rows[y].size()) +
                " tiles long where the first is " + std::to_string(width) + ", so the picture is not rectangular";
            return picture;
        }
    }

    TileGrid grid(static_cast<int>(width), static_cast<int>(rows.size()), tileSize);
    for (std::size_t y = 0; y < rows.size(); y++) {
        for (std::size_t x = 0; x < width; x++) {
            const TileCoord tile{static_cast<int>(x), static_cast<int>(y)};
            const char glyph = rows[y][x];
            const std::map<char, Terrain>::const_iterator found = legend.find(glyph);
            if (found == legend.end()) {
                // impassable and with no cover, until the caller sets otherwise
                grid.setPassable(tile, false);
                grid.setCover(tile, Cover::None);
                std::vector<Unknown>::iterator known = std::find_if(picture.unknown.begin(), picture.unknown.end(),
                    [glyph](const Unknown& entry) { return entry.glyph == glyph; });
                if (known == picture.unknown.end()) {
                    known = picture.unknown.insert(picture.unknown.end(), Unknown{glyph, {}});
                }
                known->tiles.push_back(tile);
                continue;
            }
            grid.setPassable(tile, found->second.passable);
            grid.setCover(tile, found->second.cover);
        }
    }

    picture.grid = std::move(grid);
    return picture;
}

};  // namespace v3d::grid
