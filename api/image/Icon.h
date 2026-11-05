/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Image.h>
#include <api/log/Logger.h>

#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::image {

/**
 * The sides an app's icon is drawn at, in pixels: the sizes Windows asks an icon file for, from
 * a list in a small window to a large tile.
 **/
inline constexpr std::array<unsigned int, 7> ICON_SIDES{ 16, 24, 32, 48, 64, 128, 256 };

/**
 * Draw an app's icon from an SVG document at each of ICON_SIDES, smallest first.
 *
 * The document is scaled to fit each square and centred in it, keeping its proportions.
 *
 * @return an RGBA image at each side, or none when the document cannot be read or gives itself
 *         no size
 **/
std::vector<boost::shared_ptr<Image>> drawIcon(std::string_view svg, const boost::shared_ptr<v3d::log::Logger>& logger);

/**
 * Encode a Windows icon file, the .ico format, holding one image at each of several sizes.
 *
 * An image 256 pixels a side is stored as a png, and a smaller one as a 32 bit bitmap with a mask
 * of its clear pixels, which every reader of the format takes. Every image is square, RGBA, and
 * from 1 to 256 pixels a side, and no two are the same size. They are stored in the order given.
 *
 * @return the file's bytes, or nothing when there are no images, one breaks a rule above, or one
 *         cannot be encoded
 **/
std::optional<std::string> encodeIcon(std::span<const boost::shared_ptr<Image>> images, const boost::shared_ptr<v3d::log::Logger>& logger);

};  // namespace v3d::image
