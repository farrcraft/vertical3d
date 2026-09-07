/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TileGrid.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>

namespace v3d::grid {

namespace {

std::string describe(TileCoord tile, int width, int height) {
    return "tile (" + std::to_string(tile.x) + ", " + std::to_string(tile.y) + ") is not on a " +
        std::to_string(width) + "x" + std::to_string(height) + " grid";
}

};  // namespace

std::string_view toString(Cover cover) {
    switch (cover) {
        case Cover::None:
            return "none";
        case Cover::Half:
            return "half";
        case Cover::Full:
            return "full";
    }
    return "?";
}

TileGrid::TileGrid(int width, int height, float tileSize) :
    width_(width),
    height_(height),
    tileSize_(tileSize) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("TileGrid dimensions must be positive, got " +
            std::to_string(width) + "x" + std::to_string(height));
    }
    if (tileSize <= 0.0f) {
        throw std::invalid_argument("TileGrid tile size must be positive, got " + std::to_string(tileSize));
    }

    const std::size_t tiles = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    passable_.assign(tiles, 1);
    cover_.assign(tiles, Cover::None);
}

int TileGrid::width() const {
    return width_;
}

int TileGrid::height() const {
    return height_;
}

float TileGrid::tileSize() const {
    return tileSize_;
}

bool TileGrid::contains(TileCoord tile) const {
    return tile.x >= 0 && tile.y >= 0 && tile.x < width_ && tile.y < height_;
}

glm::vec3 TileGrid::tileToWorld(TileCoord tile) const {
    const glm::vec3 corner = worldMin();
    return glm::vec3(corner.x + (static_cast<float>(tile.x) + 0.5f) * tileSize_, 0.0f,
        corner.z + (static_cast<float>(tile.y) + 0.5f) * tileSize_);
}

TileCoord TileGrid::worldToTile(const glm::vec3& position) const {
    const glm::vec3 corner = worldMin();
    return TileCoord{ static_cast<int>(std::floor((position.x - corner.x) / tileSize_)),
        static_cast<int>(std::floor((position.z - corner.z) / tileSize_)) };
}

glm::vec3 TileGrid::worldMin() const {
    return glm::vec3(-0.5f * static_cast<float>(width_) * tileSize_, 0.0f,
        -0.5f * static_cast<float>(height_) * tileSize_);
}

bool TileGrid::passable(TileCoord tile) const {
    return contains(tile) && passable_[index(tile)] != 0;
}

void TileGrid::setPassable(TileCoord tile, bool passable) {
    if (!contains(tile)) {
        throw std::out_of_range(describe(tile, width_, height_));
    }
    passable_[index(tile)] = passable ? 1 : 0;
}

Cover TileGrid::cover(TileCoord tile) const {
    return contains(tile) ? cover_[index(tile)] : Cover::None;
}

void TileGrid::setCover(TileCoord tile, Cover cover) {
    if (!contains(tile)) {
        throw std::out_of_range(describe(tile, width_, height_));
    }
    cover_[index(tile)] = cover;
}

std::size_t TileGrid::index(TileCoord tile) const {
    return static_cast<std::size_t>(tile.y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(tile.x);
}

};  // namespace v3d::grid
