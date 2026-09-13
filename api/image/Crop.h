/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <boost/shared_ptr.hpp>

#include "Image.h"

namespace v3d::image {

/**
 * Take a rectangle out of an image.
 *
 * This is the counterpart of `TextureAtlas::region(x, y, w, h, data, stride)`, which blits a
 * rectangle *into* an image and had none: a sheet could be packed and never unpacked. Cutting
 * a sprite back out of a hand-packed sheet is what an app adopting a packer has to do once,
 * and doing it outside the tree means an image codec outside the tree.
 *
 * The cut image carries the source's bits per pixel; nothing is converted.
 *
 * @param source the image to cut from, which is not modified
 * @param x the rectangle's left edge in the source's pixels
 * @param y its top edge
 * @return the cut image, or null when the rectangle is empty, the source has no depth, or
 *         the rectangle is not wholly inside the source - a partly outside crop would read
 *         whatever follows a row, which is a picture rather than an error
 **/
boost::shared_ptr<Image> crop(const Image & source, unsigned int x, unsigned int y, unsigned int width, unsigned int height);

};  // namespace v3d::image
