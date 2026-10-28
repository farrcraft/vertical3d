/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bmp.h"

#include <api/image/Image.h>
#include <api/image/Writer.h>
#include <api/image/format/BmpFileHeader.h>
#include <api/image/format/BmpInfoHeader.h>
#include <api/image/format/BmpRgbQuad.h>
#include <api/image/Channels.h>
#include <api/log/Logger.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::image::writer {

/**
 **/
Bmp::Bmp(const boost::shared_ptr<v3d::log::Logger>& logger) : Writer(logger) {
}

std::optional<std::string> Bmp::encode(const boost::shared_ptr<Image>& img) {
    if (!img) {
        return std::nullopt;
    }

    // an 8 bit bmp is palettised, and its pixels are indices into the palette. A grey one
    // carries a ramp whose nth entry is the grey n. That makes the index and the grey
    // the same number, and the pixel data goes out unchanged.
    const bool grey = img->format() == v3d::image::Image::Format::Grey;
    const uint32_t shades = grey ? 256U : 0U;

    format::bmp_file_header fheader;
    memset(&fheader, 0, sizeof(format::bmp_file_header));

    fheader.type_ = format::BMP_SIGNATURE;
    fheader.offset_ = sizeof(format::bmp_file_header) + sizeof(format::bmp_info_header) + (shades * sizeof(format::bmp_rgb_quad));
    // the total file size, filled in below once the padded data length is known
    fheader.size_ = fheader.offset_;

    format::bmp_info_header iheader;
    memset(&iheader, 0, sizeof(format::bmp_info_header));

    iheader.size_ = sizeof(format::bmp_info_header);
    iheader.width_ = static_cast<int32_t>(img->width());
    iheader.height_ = static_cast<int32_t>(img->height());
    iheader.bits_ = img->bpp();
    iheader.compression_ = 0;
    // a reader takes the table's length from the count of colours used, so the count is
    // the whole ramp the table below holds
    iheader.used_ = shades;
    iheader.important_ = shades;

    int32_t width;
    int32_t pad;
    width = pad = iheader.width_ * (iheader.bits_ / 8);
    // adjust pad width to dword boundary alignment
    while (pad % 4 != 0) {
        pad++;
    }

    const unsigned int channels = img->bpp() / 8;
    iheader.imageSize_ = img->width() * img->height() * channels;

    // size of the image data including the per row boundary padding
    const uint64_t rows = img->height();
    const uint64_t rowBytes = static_cast<uint64_t>(width);
    const uint64_t size = static_cast<uint64_t>(pad) * rows;

    fheader.size_ = static_cast<uint32_t>(fheader.offset_ + size);

    std::string encoded;
    encoded.reserve(fheader.size_);
    encoded.append(reinterpret_cast<const char*>(&fheader), sizeof(format::bmp_file_header));
    encoded.append(reinterpret_cast<const char*>(&iheader), sizeof(format::bmp_info_header));

    // the whole ramp, as many entries as the header says the file uses
    for (uint32_t shade = 0; shade < shades; ++shade) {
        format::bmp_rgb_quad entry;
        entry.blue_ = entry.green_ = entry.red_ = static_cast<unsigned char>(shade);
        entry.reserved_ = 0;
        encoded.append(reinterpret_cast<const char*>(&entry), sizeof(format::bmp_rgb_quad));
    }

    std::vector<unsigned char> scratch(size);
    unsigned char* data = scratch.data();
    const unsigned char* temp = img->data();

    // each row is copied on its own, because the padding is per row and the source
    // image has none of it. A single index over both buffers would run off the end of
    // each. The height is written positive, which means the rows are stored bottom up, so
    // the image's top row goes last
    for (uint64_t row = 0; row < rows; ++row) {
        unsigned char* dest = data + ((rows - 1 - row) * pad);
        const unsigned char* src = temp + (row * rowBytes);
        if (grey) {
            // an index is one byte and has no channel order to correct
            memcpy(dest, src, static_cast<size_t>(rowBytes));
            continue;
        }
        // rgb in memory, bgr on disk
        swapRedBlue(src, dest, {.pixels = img->width(), .channels = channels});
    }

    encoded.append(reinterpret_cast<const char*>(data), static_cast<std::size_t>(size));
    return encoded;
}

};  // namespace v3d::image::writer
