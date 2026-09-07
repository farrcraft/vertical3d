/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Overlay.h"

#include <array>
#include <cstddef>

namespace v3d::grid {

std::array<glm::vec3, 4> tileCorners(const TileGrid& grid, TileCoord tile) {
    const float half = 0.5f * grid.tileSize();
    const glm::vec3 centre = grid.tileToWorld(tile);
    const float lift = centre.y + OVERLAY_LIFT;

    return {
        glm::vec3(centre.x - half, lift, centre.z - half),
        glm::vec3(centre.x + half, lift, centre.z - half),
        glm::vec3(centre.x + half, lift, centre.z + half),
        glm::vec3(centre.x - half, lift, centre.z + half)
    };
}

void outlineTile(const TileGrid& grid, TileCoord tile, const glm::vec4& colour, const LineSink& sink) {
    if (!sink || !grid.contains(tile)) {
        return;
    }

    const std::array<glm::vec3, 4> corners = tileCorners(grid, tile);
    for (std::size_t corner = 0; corner < corners.size(); ++corner) {
        sink(corners[corner], corners[(corner + 1) % corners.size()], colour);
    }
}

void outlineGrid(const TileGrid& grid, const glm::vec4& interior, const glm::vec4& border, const LineSink& sink) {
    if (!sink) {
        return;
    }

    const glm::vec3 origin = grid.worldMin();
    const float lift = origin.y + OVERLAY_LIFT;
    const float tileSize = grid.tileSize();
    const float spanX = static_cast<float>(grid.width()) * tileSize;
    const float spanZ = static_cast<float>(grid.height()) * tileSize;

    for (int x = 0; x <= grid.width(); ++x) {
        const float at = origin.x + static_cast<float>(x) * tileSize;
        const bool outer = x == 0 || x == grid.width();
        sink(glm::vec3(at, lift, origin.z), glm::vec3(at, lift, origin.z + spanZ), outer ? border : interior);
    }
    for (int y = 0; y <= grid.height(); ++y) {
        const float at = origin.z + static_cast<float>(y) * tileSize;
        const bool outer = y == 0 || y == grid.height();
        sink(glm::vec3(origin.x, lift, at), glm::vec3(origin.x + spanX, lift, at), outer ? border : interior);
    }
}

};  // namespace v3d::grid
