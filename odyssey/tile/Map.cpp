/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#include "Map.h"

#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace odyssey::tile {

namespace {

constexpr char floorGlyph = '.';
constexpr char wallGlyph = '#';
constexpr char crateGlyph = 'o';
constexpr char startGlyph = '@';

/**
 * @param glyph a character from a map row
 * @param kind where the kind it names is written
 * @return false when the character names no kind, which rejects the document
 **/
bool kindFromGlyph(char glyph, Kind* kind) {
    switch (glyph) {
    case floorGlyph:
    case startGlyph:
        *kind = Kind::Floor;
        return true;
    case wallGlyph:
        *kind = Kind::Wall;
        return true;
    case crateGlyph:
        *kind = Kind::Crate;
        return true;
    default:
        return false;
    }
}

/**
 * What a kind is to the grid. Passability and cover are independent flags, and a kind is
 * where this app decides both at once - see v3d::grid::Cover.
 **/
bool passable(Kind kind) {
    return kind == Kind::Floor;
}

v3d::grid::Cover cover(Kind kind) {
    switch (kind) {
    case Kind::Wall:
        return v3d::grid::Cover::Full;
    case Kind::Crate:
        return v3d::grid::Cover::Half;
    default:
        return v3d::grid::Cover::None;
    }
}

};  // namespace

/**
 **/
Map::Map(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
}

/**
 **/
bool Map::load(const boost::shared_ptr<v3d::asset::kind::Json>& document) {
    if (!document) {
        logger_->get()->error("there is no map document to read");
        return false;
    }
    auto const doc = document->document();
    if (!doc.contains("tiles") || !doc.at("tiles").is_array()) {
        logger_->get()->error("the map has no tiles array");
        return false;
    }
    const boost::json::array& rows = doc.at("tiles").as_array();
    if (rows.empty()) {
        logger_->get()->error("the map has no rows");
        return false;
    }

    std::vector<std::string> lines;
    for (auto const& row : rows) {
        if (!row.is_string()) {
            logger_->get()->error("a map row is not a string");
            return false;
        }
        lines.push_back(boost::json::value_to<std::string>(row));
    }

    const std::size_t width = lines.front().size();
    if (width == 0) {
        logger_->get()->error("the map's first row is empty");
        return false;
    }
    for (const std::string& line : lines) {
        if (line.size() != width) {
            logger_->get()->error("the map's rows are {} and {} tiles long, so it is not rectangular",
                width, line.size());
            return false;
        }
    }

    // read the whole document before touching this map's own state, so a rejected one
    // leaves whatever was loaded before intact rather than half replaced
    std::vector<Kind> kinds;
    kinds.reserve(width * lines.size());
    v3d::grid::TileCoord start{-1, -1};
    for (std::size_t y = 0; y < lines.size(); y++) {
        for (std::size_t x = 0; x < width; x++) {
            const char glyph = lines[y][x];
            Kind kind = Kind::Floor;
            if (!kindFromGlyph(glyph, &kind)) {
                logger_->get()->error("the map has an unknown tile '{}' at {}, {}", glyph, x, y);
                return false;
            }
            if (glyph == startGlyph) {
                start = v3d::grid::TileCoord{static_cast<int>(x), static_cast<int>(y)};
            }
            kinds.push_back(kind);
        }
    }

    auto grid = boost::make_shared<v3d::grid::TileGrid>(
        static_cast<int>(width), static_cast<int>(lines.size()));
    for (std::size_t i = 0; i < kinds.size(); i++) {
        const v3d::grid::TileCoord tile{
            static_cast<int>(i % width), static_cast<int>(i / width)};
        grid->setPassable(tile, passable(kinds[i]));
        grid->setCover(tile, cover(kinds[i]));
        if (start.x < 0 && passable(kinds[i])) {
            start = tile;
        }
    }
    if (start.x < 0) {
        logger_->get()->error("the map has nowhere to stand");
        return false;
    }

    grid_ = grid;
    kinds_ = kinds;
    start_ = start;
    return true;
}

/**
 **/
bool Map::loaded() const noexcept {
    return grid_ != nullptr;
}

/**
 **/
const boost::shared_ptr<v3d::grid::TileGrid>& Map::grid() const noexcept {
    return grid_;
}

/**
 **/
Kind Map::kind(v3d::grid::TileCoord tile) const {
    if (!grid_ || !grid_->contains(tile)) {
        return Kind::Wall;
    }
    return kinds_[static_cast<std::size_t>(tile.y) * static_cast<std::size_t>(grid_->width()) +
        static_cast<std::size_t>(tile.x)];
}

/**
 **/
v3d::grid::TileCoord Map::start() const noexcept {
    return start_;
}

};  // namespace odyssey::tile
