/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bmp.h"

#include <api/image/BmpFileHeader.h>
#include <api/image/BmpInfoHeader.h>
#include <api/image/BmpRgbQuad.h>

#include <bit>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::image::reader {
/**
 **/
Bmp::Bmp(const boost::shared_ptr<v3d::log::Logger>& logger) : Reader(logger) {
}

namespace {

void logHeaders(const boost::shared_ptr<v3d::log::Logger>& logger, const bmp_file_header& fheader,
    const bmp_info_header& iheader, int num_colors) {
    logger->get()->debug("BMPReader::read - bmp type: {}", fheader.type_);
    logger->get()->debug("BMPReader::read - bmp size: {}", fheader.size_);
    logger->get()->debug("BMPReader::read - bmp reserved1: {}", fheader.reserved1_);
    logger->get()->debug("BMPReader::read - bmp reserved2: {}", fheader.reserved2_);
    logger->get()->debug("BMPReader::read - bmp offset: {}", fheader.offset_);
    logger->get()->debug("BMPReader::read - info size: {}", iheader.size_);
    logger->get()->debug("BMPReader::read - info width: {}", iheader.width_);
    logger->get()->debug("BMPReader::read - info height: {}", iheader.height_);
    logger->get()->debug("BMPReader::read - info planes: {}", iheader.planes_);
    logger->get()->debug("BMPReader::read - info bits: {}", iheader.bits_);
    logger->get()->debug("BMPReader::read - info compression: {}", iheader.compression_);
    logger->get()->debug("BMPReader::read - info image size: {}", iheader.imageSize_);
    logger->get()->debug("BMPReader::read - info xppm: {}", iheader.xppm_);
    logger->get()->debug("BMPReader::read - info yppm: {}", iheader.yppm_);
    logger->get()->debug("BMPReader::read - info used: {}", iheader.used_);
    logger->get()->debug("BMPReader::read - info important: {}", iheader.important_);
    logger->get()->debug("BMPReader::read - num colors: {}", num_colors);
}

/**
 * One channel of a packed pixel, picked out by its mask and widened to a byte. A mask of
 * five bits gives 0 to 31, which is spread over 0 to 255 so that full is 255.
 **/
unsigned char channel(uint32_t pixel, uint32_t mask) {
    if (mask == 0) {
        return 0;
    }
    uint32_t shift = 0;
    while (((mask >> shift) & 1u) == 0) {
        shift++;
    }
    const uint32_t width = static_cast<uint32_t>(std::popcount(mask));
    const uint32_t value = (pixel & mask) >> shift;
    if (width >= 8) {
        return static_cast<unsigned char>(value >> (width - 8));
    }
    const uint32_t most = (1u << width) - 1u;
    return static_cast<unsigned char>((value * 255u + most / 2) / most);
}

/**
 * Where each channel sits in a packed 16 or 32 bit pixel.
 **/
struct Masks final {
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint32_t alpha = 0;
};

uint32_t readWord(const unsigned char* at) {
    return static_cast<uint32_t>(at[0]) | (static_cast<uint32_t>(at[1]) << 8) |
        (static_cast<uint32_t>(at[2]) << 16) | (static_cast<uint32_t>(at[3]) << 24);
}

/**
 * Where a bmp's rows are and how they are laid out.
 **/
struct Layout final {
    unsigned int bits = 0;
    uint64_t columns = 0;
    uint64_t rows = 0;
    uint64_t pad = 0;       /**< the bytes a row takes on disk, padded to a dword boundary **/
    bool bottomUp = true;   /**< a positive height stores the last row first, which is the usual way **/
    unsigned int channels = 3;
};

const uint32_t bitfields = 3;

/**
 * Whether the reader has a conversion for this file: 8, 16, 24 or 32 bits, uncompressed or, at
 * 16 and 32 bits, packed by masks, and with pixels to read.
 **/
bool supported(const bmp_info_header& header, const boost::shared_ptr<v3d::log::Logger>& logger) {
    const unsigned int bits = header.bits_;
    if (bits != 8 && bits != 16 && bits != 24 && bits != 32) {
        logger->get()->error("BMPReader::read - {} bit bmps are not supported", bits);
        return false;
    }
    const bool packed = header.compression_ == bitfields && (bits == 16 || bits == 32);
    if (header.compression_ != 0 && !packed) {
        logger->get()->error("BMPReader::read - compression {} is not supported", header.compression_);
        return false;
    }
    if (header.width_ <= 0 || header.height_ == 0 || header.size_ < sizeof(bmp_info_header)) {
        logger->get()->error("BMPReader::read - the bmp has no pixels, or a header too small to be one");
        return false;
    }
    return true;
}

/**
 * Where each channel of a 16 or 32 bit pixel is: the standard places for an uncompressed file,
 * or the masks the file gives. A 40 byte header is followed by three masks, and a longer one
 * holds them and an alpha mask too.
 **/
bool readMasks(const unsigned char* encoded, std::size_t length, const bmp_info_header& header, Masks* masks) {
    if (header.bits_ == 16) {
        // five bits a channel, which is what an uncompressed 16 bit file means
        *masks = Masks{0x7C00u, 0x03E0u, 0x001Fu, 0u};
    } else if (header.bits_ == 32) {
        *masks = Masks{0x00FF0000u, 0x0000FF00u, 0x000000FFu, 0xFF000000u};
    }
    if (header.compression_ != bitfields) {
        return true;
    }
    const std::size_t held = sizeof(bmp_file_header) + sizeof(bmp_info_header);
    const bool alpha = header.size_ >= sizeof(bmp_info_header) + 16;
    if (length < held + (alpha ? 16 : 12)) {
        return false;
    }
    masks->red = readWord(encoded + held);
    masks->green = readWord(encoded + held + 4);
    masks->blue = readWord(encoded + held + 8);
    masks->alpha = alpha ? readWord(encoded + held + 12) : 0u;
    return true;
}

/**
 * An 8 bit file's colour table, which follows the info header whatever that header's length.
 * A file says how many entries it uses, and none means all of them.
 **/
bool readPalette(const unsigned char* encoded, std::size_t length, const bmp_info_header& header,
    std::vector<bmp_rgb_quad>* colors) {
    const std::size_t at = sizeof(bmp_file_header) + header.size_;
    const std::size_t used = header.used_ == 0 || header.used_ > 256 ? 256 : header.used_;
    const std::size_t table = sizeof(bmp_rgb_quad) * used;
    if (length < at || length - at < table) {
        return false;
    }
    colors->assign(256, bmp_rgb_quad{});
    memcpy(colors->data(), encoded + at, table);
    return true;
}

/**
 * One pixel, as rgb or rgba.
 *
 * @return whether its alpha is anything but zero
 **/
bool convertPixel(const unsigned char* src, uint64_t column, const Layout& layout, const Masks& masks,
    const std::vector<bmp_rgb_quad>& colors, unsigned char* out) {
    if (layout.bits == 8) {
        const bmp_rgb_quad& entry = colors[src[column]];
        out[0] = entry.red_;
        out[1] = entry.green_;
        out[2] = entry.blue_;
        return false;
    }
    if (layout.bits == 24) {
        // bgr on disk, rgb in memory
        out[0] = src[column * 3 + 2];
        out[1] = src[column * 3 + 1];
        out[2] = src[column * 3];
        return false;
    }
    const unsigned char* packed = src + column * (layout.bits / 8);
    const uint32_t pixel = layout.bits == 16
        ? static_cast<uint32_t>(packed[0]) | (static_cast<uint32_t>(packed[1]) << 8)
        : readWord(packed);
    out[0] = channel(pixel, masks.red);
    out[1] = channel(pixel, masks.green);
    out[2] = channel(pixel, masks.blue);
    if (layout.channels != 4) {
        return false;
    }
    out[3] = channel(pixel, masks.alpha);
    return out[3] != 0;
}

/**
 * Every row, top down whatever order the file stored them in. A 32 bit file whose alpha is zero
 * everywhere did not use it, as most writers do not, and is made opaque rather than invisible.
 **/
void convertRows(const unsigned char* pixels, const Layout& layout, const Masks& masks,
    const std::vector<bmp_rgb_quad>& colors, unsigned char* data) {
    bool anyAlpha = false;
    for (uint64_t row = 0; row < layout.rows; ++row) {
        const unsigned char* src = pixels + (layout.bottomUp ? layout.rows - 1 - row : row) * layout.pad;
        unsigned char* dest = data + row * layout.columns * layout.channels;
        for (uint64_t column = 0; column < layout.columns; ++column) {
            anyAlpha = convertPixel(src, column, layout, masks, colors, dest + column * layout.channels) || anyAlpha;
        }
    }
    if (layout.channels == 4 && !anyAlpha) {
        for (uint64_t pixel = 0; pixel < layout.columns * layout.rows; ++pixel) {
            data[pixel * 4 + 3] = 0xFF;
        }
    }
}

};  // namespace

/**
 **/
boost::shared_ptr<Image> Bmp::read(const unsigned char* encoded, std::size_t length) {
    boost::shared_ptr<Image> empty_ptr;
    if (encoded == nullptr) {
        return empty_ptr;
    }

    // the two headers come first, one after the other
    bmp_file_header fheader;
    memset(&fheader, 0, sizeof(bmp_file_header));
    bmp_info_header iheader;
    memset(&iheader, 0, sizeof(bmp_info_header));
    if (length < sizeof(bmp_file_header) + sizeof(bmp_info_header)) {
        logger_->get()->error("BMPReader::read - error reading bmp headers!");
        return empty_ptr;
    }
    memcpy(&fheader, encoded, sizeof(bmp_file_header));
    memcpy(&iheader, encoded + sizeof(bmp_file_header), sizeof(bmp_info_header));

    if (fheader.type_ != 19778) {
        logger_->get()->error("BMPReader::read - bad header magic number!");
        return empty_ptr;
    }
    // checked before anything is sized from it: the row arithmetic below assumes a whole
    // number of bytes a pixel and a conversion that knows the layout
    if (!supported(iheader, logger_)) {
        return empty_ptr;
    }
    logHeaders(logger_, fheader, iheader, iheader.bits_ == 8 ? 256 : 0);

    Masks masks;
    if (!readMasks(encoded, length, iheader, &masks)) {
        logger_->get()->error("BMPReader::read - error reading bmp masks!");
        return empty_ptr;
    }
    std::vector<bmp_rgb_quad> colors;
    if (iheader.bits_ == 8 && !readPalette(encoded, length, iheader, &colors)) {
        logger_->get()->error("BMPReader::read - error reading bmp colors!");
        return empty_ptr;
    }

    Layout layout;
    layout.bits = iheader.bits_;
    layout.columns = static_cast<uint64_t>(iheader.width_);
    layout.pad = (layout.columns * (layout.bits / 8) + 3) / 4 * 4;
    layout.bottomUp = iheader.height_ > 0;
    layout.rows = static_cast<uint64_t>(layout.bottomUp ? iheader.height_ : -static_cast<int64_t>(iheader.height_));
    // 32 bits keeps its alpha; everything else comes back as rgb
    layout.channels = layout.bits == 32 ? 4 : 3;

    // the pixels are where the file header says, which is after the palette or the masks
    const std::size_t start = fheader.offset_;
    if (start > length || length - start < layout.pad * layout.rows) {
        logger_->get()->error("BMPReader::read - error reading bmp data!");
        return empty_ptr;
    }

    boost::shared_ptr<Image> image(new Image(static_cast<uint32_t>(layout.columns), static_cast<uint32_t>(layout.rows),
        static_cast<uint8_t>(layout.channels * 8)));
    convertRows(encoded + start, layout, masks, colors, image->data());
    return image;
}

};  // namespace v3d::image::reader
