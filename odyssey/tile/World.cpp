/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#include "World.h"

#include <api/asset/Json.h>
#include <api/grid/TileGrid.h>
#include <api/type/Checked.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <expected>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/weak_ptr.hpp>
#include <glm/vec3.hpp>

namespace odyssey::tile {

namespace {

/**
 * The largest corner or size a world document may give, in tiles. A float holds every whole
 * number up to 2^24 exactly, and a world this large is far beyond any board odyssey draws.
 **/
constexpr int32_t largest = 1 << 20;

/**
 * Read an "x" and "y" pair of whole numbers from a member of an object.
 *
 * @return the pair, or nothing when the member is absent, either number is missing, not a
 *         whole number, or outside [minimum, largest]
 **/
std::optional<v3d::grid::TileCoord> readPair(const boost::json::object& object, std::string_view key, int32_t minimum) {
    const boost::json::object* pair = v3d::asset::readObject(object, key);
    if (pair == nullptr) {
        return std::nullopt;
    }
    std::optional<int32_t> values[2];
    const char* const names[2] = {"x", "y"};
    for (int axis = 0; axis < 2; axis++) {
        const std::optional<double> number = v3d::asset::readNumber(*pair, names[axis]);
        if (!number || !std::isfinite(*number) || std::floor(*number) != *number) {
            return std::nullopt;
        }
        values[axis] = v3d::type::toInteger(static_cast<float>(*number), minimum, largest);
        if (!values[axis]) {
            return std::nullopt;
        }
    }
    return v3d::grid::TileCoord{*values[0], *values[1]};
}

/**
 * @return whether two rectangles of tiles share a tile
 **/
bool overlap(v3d::grid::TileCoord cornerA, v3d::grid::TileCoord sizeA, v3d::grid::TileCoord cornerB, v3d::grid::TileCoord sizeB) {
    return cornerA.x < cornerB.x + sizeB.x && cornerB.x < cornerA.x + sizeA.x &&
        cornerA.y < cornerB.y + sizeB.y && cornerB.y < cornerA.y + sizeA.y;
}

};  // namespace

/**
 **/
World::World(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assets,
    const boost::shared_ptr<v3d::asset::Worker>& worker) :
    logger_(logger),
    assets_(assets),
    worker_(worker),
    regions_(boost::make_shared<std::vector<Region>>()) {
}

/**
 **/
bool World::load(const boost::shared_ptr<v3d::asset::kind::Json>& document) {
    if (!document) {
        logger_->get()->error("there is no world document to read");
        return false;
    }
    const boost::json::object& root = document->document();
    const boost::json::array* listed = v3d::asset::readArray(root, "regions");
    if (listed == nullptr || listed->empty()) {
        logger_->get()->error("the world has no regions array");
        return false;
    }

    // read the whole document before touching this world's own state, so a rejected one
    // leaves whatever was loaded before intact
    std::vector<Region> regions;
    for (std::size_t index = 0; index < listed->size(); index++) {
        const boost::json::value& entry = (*listed)[index];
        if (!entry.is_object()) {
            logger_->get()->error("region {} is not an object", index);
            return false;
        }
        const std::optional<std::string> file = v3d::asset::readString(entry.as_object(), "file");
        const std::optional<v3d::grid::TileCoord> corner = readPair(entry.as_object(), "corner", -largest);
        const std::optional<v3d::grid::TileCoord> size = readPair(entry.as_object(), "size", 1);
        if (!file || !corner || !size) {
            logger_->get()->error("region {} needs a file, a corner and a size of whole numbers within {}", index, largest);
            return false;
        }
        for (std::size_t other = 0; other < regions.size(); other++) {
            if (overlap(*corner, *size, regions[other].corner, regions[other].size)) {
                logger_->get()->error("region {} overlaps region {}", index, other);
                return false;
            }
        }
        Region region;
        region.file = *file;
        region.corner = *corner;
        region.size = *size;
        regions.push_back(std::move(region));
    }

    const std::optional<double> start = v3d::asset::readNumber(root, "start");
    const std::optional<int32_t> first = start && std::isfinite(*start) && std::floor(*start) == *start ?
        v3d::type::toInteger(static_cast<float>(*start), 0, static_cast<int32_t>(regions.size()) - 1) : std::nullopt;
    if (!first) {
        logger_->get()->error("the world's start is not the index of one of its regions");
        return false;
    }
    Region& home = regions[static_cast<std::size_t>(*first)];
    home.map = read(logger_, assets_, home.file, home.corner, home.size);
    if (!home.map) {
        return false;
    }

    // the old regions go with the old vector, so a completion still on the worker for one of
    // them finds nothing to fill
    regions_ = boost::make_shared<std::vector<Region>>(std::move(regions));
    start_ = world(static_cast<std::size_t>(*first), home.map->start());
    min_ = regions_->front().corner;
    max_ = regions_->front().corner;
    for (const Region& region : *regions_) {
        min_ = v3d::grid::TileCoord{std::min(min_.x, region.corner.x), std::min(min_.y, region.corner.y)};
        max_ = v3d::grid::TileCoord{std::max(max_.x, region.corner.x + region.size.x), std::max(max_.y, region.corner.y + region.size.y)};
    }
    look(start_);
    return true;
}

/**
 **/
boost::shared_ptr<const Map> World::read(const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<v3d::asset::Manager>& assets, const std::string& file, v3d::grid::TileCoord corner,
    v3d::grid::TileCoord size) {
    const boost::shared_ptr<v3d::asset::kind::Json> document = assets->load<v3d::asset::kind::Json>(file);
    boost::shared_ptr<Map> map = boost::make_shared<Map>(logger);
    const glm::vec3 placed(static_cast<float>(corner.x) * Map::TILE_SIZE, 0.0f, static_cast<float>(corner.y) * Map::TILE_SIZE);
    if (!map->load(document, placed)) {
        logger->get()->error("the region {} could not be read", file);
        return nullptr;
    }
    if (map->grid()->width() != size.x || map->grid()->height() != size.y) {
        logger->get()->error("the region {} is {} x {} tiles, and the world says {} x {}", file, map->grid()->width(),
            map->grid()->height(), size.x, size.y);
        return nullptr;
    }
    return map;
}

/**
 **/
v3d::grid::TileCoord World::start() const noexcept {
    return start_;
}

/**
 **/
std::size_t World::regions() const noexcept {
    return regions_->size();
}

/**
 **/
std::optional<std::size_t> World::regionAt(v3d::grid::TileCoord tile) const {
    for (std::size_t index = 0; index < regions_->size(); index++) {
        if (distance((*regions_)[index], tile) == 0) {
            return index;
        }
    }
    return std::nullopt;
}

/**
 **/
boost::shared_ptr<const Map> World::map(std::size_t region) const {
    return regions_->at(region).map;
}

/**
 **/
const Sight& World::sight(std::size_t region) const {
    return regions_->at(region).sight;
}

/**
 **/
v3d::grid::TileCoord World::corner(std::size_t region) const {
    return regions_->at(region).corner;
}

/**
 **/
v3d::grid::TileCoord World::local(std::size_t region, v3d::grid::TileCoord tile) const {
    const v3d::grid::TileCoord placed = regions_->at(region).corner;
    return v3d::grid::TileCoord{tile.x - placed.x, tile.y - placed.y};
}

/**
 **/
v3d::grid::TileCoord World::world(std::size_t region, v3d::grid::TileCoord tile) const {
    const v3d::grid::TileCoord placed = regions_->at(region).corner;
    return v3d::grid::TileCoord{tile.x + placed.x, tile.y + placed.y};
}

/**
 **/
bool World::passable(v3d::grid::TileCoord tile) const {
    const std::optional<std::size_t> region = regionAt(tile);
    if (!region || !(*regions_)[*region].map) {
        return false;
    }
    return (*regions_)[*region].map->grid()->passable(local(*region, tile));
}

/**
 **/
v3d::grid::TileCoord World::min() const noexcept {
    return min_;
}

/**
 **/
v3d::grid::TileCoord World::max() const noexcept {
    return max_;
}

/**
 **/
v3d::grid::TileCoord World::view(v3d::grid::TileCoord player, v3d::grid::TileCoord screen) const {
    // centred on the player, then held inside the world, then at its least tile when the world
    // is the smaller of the two
    const int x = std::max(std::min(player.x - (screen.x / 2), max_.x - screen.x), min_.x);
    const int y = std::max(std::min(player.y - (screen.y / 2), max_.y - screen.y), min_.y);
    return v3d::grid::TileCoord{x, y};
}

/**
 **/
bool World::loaded(std::size_t region) const {
    return regions_->at(region).map != nullptr;
}

/**
 **/
bool World::loading(std::size_t region) const {
    return regions_->at(region).loading;
}

/**
 **/
int World::distance(const Region& region, v3d::grid::TileCoord tile) {
    // how far outside the region's span the tile lies on each axis, and zero inside it
    const int dx = std::max({region.corner.x - tile.x, tile.x - (region.corner.x + region.size.x - 1), 0});
    const int dy = std::max({region.corner.y - tile.y, tile.y - (region.corner.y + region.size.y - 1), 0});
    return std::max(dx, dy);
}

/**
 **/
void World::stream(v3d::grid::TileCoord player) {
    const std::optional<std::size_t> standing = regionAt(player);
    for (std::size_t index = 0; index < regions_->size(); index++) {
        const Region& region = (*regions_)[index];
        const int away = distance(region, player);
        if (away <= APPROACH && !region.map && !region.loading && !region.failed) {
            request(index);
        } else if (away > DEPART && standing != index) {
            release(index);
        }
    }
}

/**
 **/
void World::request(std::size_t index) {
    Region& region = (*regions_)[index];
    region.loading = true;
    const unsigned int generation = ++region.generation;
    worker_->post(
        [logger = logger_, assets = assets_, file = region.file, corner = region.corner, size = region.size]() {
            return read(logger, assets, file, corner, size);
        },
        [regions = boost::weak_ptr<std::vector<Region>>(regions_), index, generation](
            std::expected<boost::shared_ptr<const Map>, std::exception_ptr> result) {
            const boost::shared_ptr<std::vector<Region>> held = regions.lock();
            if (!held || index >= held->size() || (*held)[index].generation != generation) {
                return;
            }
            Region& arrived = (*held)[index];
            arrived.loading = false;
            // a map that could not be read was reported by read(), and leaves the region
            // unloaded until the player has gone away from it and come back
            if (result && *result) {
                arrived.map = *result;
            } else {
                arrived.failed = true;
            }
        });
}

/**
 **/
void World::release(std::size_t index) {
    Region& region = (*regions_)[index];
    if (!region.map && !region.loading && !region.failed) {
        return;
    }
    region.map.reset();
    region.loading = false;
    region.failed = false;
    // a load still on the worker is dropped when it arrives
    region.generation++;
}

/**
 **/
void World::look(v3d::grid::TileCoord player) {
    const std::optional<std::size_t> region = regionAt(player);
    for (std::size_t index = 0; index < regions_->size(); index++) {
        if (index != region) {
            (*regions_)[index].sight.blind();
        }
    }
    if (!region || !(*regions_)[*region].map) {
        return;
    }
    Region& here = (*regions_)[*region];
    here.sight.look(*here.map->grid(), local(*region, player));
}

};  // namespace odyssey::tile
