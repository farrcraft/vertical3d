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

#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::image::writer {

/**
 **/
Bmp::Bmp(const boost::shared_ptr<v3d::log::Logger>& logger) : Writer(logger) {
}

bool Bmp::write(std::string_view filename, const boost::shared_ptr<Image>& img) {
    if (!img) {
        return false;
    }

    // an 8 bit bmp is palettised - its pixels are indices rather than colours - so a grey
    // one carries a ramp whose nth entry is the grey n. That makes the index and the grey
    // the same number, and the pixel data goes out unchanged.
    const bool grey = img->format() == v3d::image::Image::Format::Grey;
    const uint32_t shades = grey ? 256U : 0U;

    std::fstream file;
    file.open(static_cast<std::string>(filename).c_str(), std::fstream::out | std::fstream::binary);

    if (file.fail()) {
        return false;
    }

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

    // write the headers
    file.write(reinterpret_cast<char*>(&fheader), sizeof(format::bmp_file_header));
    file.write(reinterpret_cast<char*>(&iheader), sizeof(format::bmp_info_header));

    // the whole ramp, as many entries as the header says the file uses
    for (uint32_t shade = 0; shade < shades; ++shade) {
        format::bmp_rgb_quad entry;
        entry.blue_ = entry.green_ = entry.red_ = static_cast<unsigned char>(shade);
        entry.reserved_ = 0;
        file.write(reinterpret_cast<char*>(&entry), sizeof(format::bmp_rgb_quad));
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

    // write image data
    file.write(reinterpret_cast<char*>(data), static_cast<std::streamsize>(size));

    file.close();

    return true;
}

};  // namespace v3d::image::writer
