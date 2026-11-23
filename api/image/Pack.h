/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>

#include <vector>

#include <boost/shared_ptr.hpp>

#include "Image.h"
#include "Packing.h"
#include "Placement.h"
#include "Sprite.h"
#include "Unpacking.h"

namespace v3d::image {

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
 * @param sprites the named images to pack, which are not modified
 * @param largest the longest side the sheet may have
 * @param logger where the atlas writes its errors
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
