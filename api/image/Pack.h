/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

#include "Image.h"

namespace v3d::image {

/**
 * One image to pack into a sheet, or one cut back out of it, and the name it goes under.
 **/
class Sprite final {
 public:
    std::string name;
    boost::shared_ptr<Image> image;
};

/**
 * Where a sprite sits in a sheet: x, y, width and height, in the sheet's pixels from its top
 * left corner.
 **/
class Placement final {
 public:
    std::string name;
    glm::uvec4 area = glm::uvec4(0);
};

/**
 * What pack() made: the sheet and where each sprite went, or the reason there is no sheet.
 **/
class Packing final {
 public:
    /** Null when the sprites were refused. **/
    boost::shared_ptr<Image> sheet;
    /** One for each sprite, in the order the sprites were given. **/
    std::vector<Placement> placements;
    /** Empty when there is a sheet. **/
    std::string refusal;
};

/**
 * What unpack() cut: one sprite for each placement, or the reason there are none.
 **/
class Unpacking final {
 public:
    /** One for each placement, in the order the placements were given. **/
    std::vector<Sprite> sprites;
    /** Empty when every placement was cut. **/
    std::string refusal;
};

/**
 * Pack images into one square sheet, through a TextureAtlas.
 *
 * Each size is tried in turn, from 16 pixels a side and doubling, then `largest` itself, and
 * the first that holds every sprite is kept. The tallest sprites are placed first, which packs a
 * skyline tighter. Every sprite has the atlas's one-texel gutter around it, and the gutter is
 * zero.
 *
 * Sprites are refused, with a reason, when there are none, when one has no image or no
 * pixels, or when two share a name. They are also refused when their depths differ or the
 * depth is not 8, 24 or 32 bits. Last, they are refused when `largest` is under 16, or when
 * they do not all fit in `largest`.
 *
 * @param largest the longest side the sheet may have
 **/
Packing pack(const std::vector<Sprite> & sprites, unsigned int largest, const boost::shared_ptr<v3d::log::Logger> & logger);

/**
 * Cut every placement out of a sheet, through crop().
 *
 * The placements are refused, with a reason, when one is empty or not wholly inside the
 * sheet. Nothing is cut then.
 **/
Unpacking unpack(const Image & sheet, const std::vector<Placement> & placements);

};  // namespace v3d::image
