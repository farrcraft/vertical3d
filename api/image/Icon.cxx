/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Icon.h"

#include <api/image/Image.h>
#include <api/image/reader/Svg.h>
#include <api/image/writer/Png.h>
#include <api/log/Logger.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::image {

namespace {

/** the longest side an icon file can state **/
constexpr std::uint32_t LARGEST = 256;

/** how long the file's header is, and each image's entry in the directory after it **/
constexpr std::uint32_t HEADER = 6;
constexpr std::uint32_t ENTRY = 16;

/** the file's header says it holds icons rather than cursors **/
constexpr std::uint16_t ICON_TYPE = 1;

/** an entry's colour planes and bits per pixel, for four bytes a pixel **/
constexpr std::uint16_t PLANES = 1;
constexpr std::uint16_t BITS = 32;

/** how long a bitmap entry's header is **/
constexpr std::uint32_t BITMAP_HEADER = 40;

/**
 * Append a number to the file, least significant byte first, as the format stores every number.
 **/
void put(std::string* bytes, std::uint32_t value, std::size_t size) {
    for (std::size_t index = 0; index < size; index++) {
        bytes->push_back(static_cast<char>((value >> (index * 8)) & 0xFFU));
    }
}

/**
 * Encode one image as a bitmap entry. A header comes first, then the pixels as blue, green, red
 * and alpha from the bottom row up. Then comes a mask of one bit a pixel, set where the pixel is
 * clear, for a reader that ignores alpha. The header states twice the height, since the pixels
 * and the mask are stacked. Each row of the mask is padded to four bytes.
 **/
std::string bitmap(const Image& image) {
    const std::uint32_t side = image.width();
    const std::uint32_t maskRow = ((side + 31) / 32) * 4;
    std::string bytes;
    put(&bytes, BITMAP_HEADER, 4);
    put(&bytes, side, 4);
    put(&bytes, side * 2, 4);
    put(&bytes, PLANES, 2);
    put(&bytes, BITS, 2);
    put(&bytes, 0, 4);  // not compressed
    put(&bytes, (side * side * 4) + (maskRow * side), 4);
    put(&bytes, 0, 4);  // no resolution
    put(&bytes, 0, 4);
    put(&bytes, 0, 4);  // no palette
    put(&bytes, 0, 4);
    for (std::uint32_t row = side; row-- > 0;) {
        for (std::uint32_t column = 0; column < side; column++) {
            const unsigned int at = ((row * side) + column) * 4;
            bytes.push_back(static_cast<char>(image[at + 2]));
            bytes.push_back(static_cast<char>(image[at + 1]));
            bytes.push_back(static_cast<char>(image[at]));
            bytes.push_back(static_cast<char>(image[at + 3]));
        }
    }
    for (std::uint32_t row = side; row-- > 0;) {
        std::string mask(maskRow, '\0');
        for (std::uint32_t column = 0; column < side; column++) {
            if (image[(((row * side) + column) * 4) + 3] == 0) {
                mask[column / 8] = static_cast<char>(mask[column / 8] | (0x80 >> (column % 8)));
            }
        }
        bytes += mask;
    }
    return bytes;
}

};  // namespace

std::vector<boost::shared_ptr<Image>> drawIcon(std::string_view svg, const boost::shared_ptr<v3d::log::Logger>& logger) {
    reader::Svg reader(logger);
    std::vector<boost::shared_ptr<Image>> drawn;
    for (const unsigned int side : ICON_SIDES) {
        boost::shared_ptr<Image> image = reader.read(reinterpret_cast<const unsigned char*>(svg.data()), svg.size(), glm::uvec2(side, side));
        if (!image) {
            return {};
        }
        drawn.push_back(std::move(image));
    }
    return drawn;
}

std::optional<std::string> encodeIcon(std::span<const boost::shared_ptr<Image>> images, const boost::shared_ptr<v3d::log::Logger>& logger) {
    if (images.empty()) {
        logger->get()->error("An icon file needs at least one image");
        return std::nullopt;
    }
    writer::Png png(logger);
    std::set<std::uint32_t> sides;
    std::vector<std::string> encoded;
    for (const boost::shared_ptr<Image>& image : images) {
        if (!image || image->format() != Image::Format::RGBA || image->width() != image->height() || image->width() == 0 ||
            image->width() > LARGEST) {
            logger->get()->error("An icon's image has to be square, RGBA and from 1 to {} pixels a side", LARGEST);
            return std::nullopt;
        }
        if (!sides.insert(image->width()).second) {
            logger->get()->error("An icon file holds one image of each size, and {} pixels came twice", image->width());
            return std::nullopt;
        }
        // the largest size is a png, which keeps it small. Every reader takes a smaller size as
        // a bitmap, and some take nothing else
        if (image->width() < LARGEST) {
            encoded.push_back(bitmap(*image));
            continue;
        }
        std::optional<std::string> made = png.encode(image);
        if (!made) {
            return std::nullopt;
        }
        encoded.push_back(std::move(*made));
    }

    std::string bytes;
    put(&bytes, 0, 2);
    put(&bytes, ICON_TYPE, 2);
    put(&bytes, static_cast<std::uint32_t>(encoded.size()), 2);
    // the images follow the directory, each where the one before it ends
    std::uint32_t offset = HEADER + (ENTRY * static_cast<std::uint32_t>(encoded.size()));
    for (std::size_t index = 0; index < encoded.size(); index++) {
        // a side of 256 does not fit in a byte, and is written as 0
        const std::uint32_t side = images[index]->width() % LARGEST;
        put(&bytes, side, 1);
        put(&bytes, side, 1);
        put(&bytes, 0, 1);  // no palette
        put(&bytes, 0, 1);  // reserved
        put(&bytes, PLANES, 2);
        put(&bytes, BITS, 2);
        put(&bytes, static_cast<std::uint32_t>(encoded[index].size()), 4);
        put(&bytes, offset, 4);
        offset += static_cast<std::uint32_t>(encoded[index].size());
    }
    for (const std::string& each : encoded) {
        bytes += each;
    }
    return bytes;
}

};  // namespace v3d::image
