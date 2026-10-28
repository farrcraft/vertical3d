/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Pack.h"

#include <api/log/Logger.h>

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

#include "Crop.h"
#include "Image.h"
#include "TextureAtlas.h"

namespace {

/** The side of the first sheet pack() tries. **/
constexpr unsigned int smallest = 16;

/** Why these sprites cannot be packed at all, whatever the size, or an empty string. **/
std::string unpackable(const std::vector<v3d::image::Sprite> & sprites) {
    if (sprites.empty()) {
        return "there are no sprites to pack";
    }
    std::set<std::string> names;
    for (const v3d::image::Sprite & sprite : sprites) {
        if (!sprite.image || sprite.image->width() == 0 || sprite.image->height() == 0) {
            return "the sprite \"" + sprite.name + "\" has no pixels";
        }
        if (!names.insert(sprite.name).second) {
            return "two sprites are named \"" + sprite.name + "\"";
        }
        if (sprite.image->bpp() != sprites.front().image->bpp()) {
            return "the sprite \"" + sprite.name + "\" is " + std::to_string(sprite.image->bpp()) + " bits a pixel and \"" +
                sprites.front().name + "\" is " + std::to_string(sprites.front().image->bpp());
        }
    }
    const unsigned int bpp = sprites.front().image->bpp();
    if (bpp != 8 && bpp != 24 && bpp != 32) {
        return "a sheet holds 8, 24 or 32 bits a pixel, not " + std::to_string(bpp);
    }
    return "";
}

/**
 * The sprites packed into one atlas of this side, in the order given by `order`, or false
 * when they do not all fit.
 **/
bool packInto(const std::vector<v3d::image::Sprite> & sprites, const std::vector<std::size_t> & order, unsigned int side,
    const boost::shared_ptr<v3d::log::Logger> & logger, v3d::image::Packing* packing) {
    const unsigned int depth = sprites.front().image->bpp() / 8U;
    v3d::image::TextureAtlas atlas(side, side, depth, logger);
    packing->placements.assign(sprites.size(), v3d::image::Placement());
    for (const std::size_t index : order) {
        const v3d::image::Sprite & sprite = sprites[index];
        const glm::ivec4 area = atlas.region(sprite.image->width(), sprite.image->height());
        if (area.x < 0) {
            return false;
        }
        atlas.region(area, sprite.image->data(), sprite.image->width() * depth);
        packing->placements[index] = {.name = sprite.name, .area = glm::uvec4(area)};
    }
    packing->sheet = atlas.image();
    return true;
}

};  // namespace

namespace v3d::image {

Packing pack(const std::vector<Sprite> & sprites, unsigned int largest, const boost::shared_ptr<v3d::log::Logger> & logger) {
    Packing packing;
    packing.refusal = unpackable(sprites);
    if (packing.refusal.empty() && largest < smallest) {
        packing.refusal = "a sheet is at least " + std::to_string(smallest) + " pixels a side, not " + std::to_string(largest);
    }
    if (!packing.refusal.empty()) {
        return packing;
    }
    // tallest first, and in the order given among sprites of one height, so a run packs the
    // same sheet every time
    std::vector<std::size_t> order(sprites.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::ranges::stable_sort(order, [&sprites](std::size_t left, std::size_t right) {
        return sprites[left].image->height() > sprites[right].image->height();
    });
    unsigned int side = smallest;
    while (!packInto(sprites, order, side, logger, &packing)) {
        if (side == largest) {
            packing.placements.clear();
            packing.refusal = "the sprites do not fit in a sheet " + std::to_string(largest) + " pixels a side";
            return packing;
        }
        // a largest side that is not a power of two is tried itself, after the last power
        // below it
        side = side > largest / 2U ? largest : side * 2U;
    }
    return packing;
}

Unpacking unpack(const Image & sheet, const std::vector<Placement> & placements) {
    Unpacking unpacking;
    for (const Placement & placement : placements) {
        const boost::shared_ptr<Image> cut = crop(sheet, placement.area.x, placement.area.y, placement.area.z, placement.area.w);
        if (!cut) {
            unpacking.sprites.clear();
            unpacking.refusal = "the sprite \"" + placement.name + "\" is not a rectangle inside the sheet";
            return unpacking;
        }
        unpacking.sprites.push_back({.name = placement.name, .image = cut});
    }
    return unpacking;
}

};  // namespace v3d::image
