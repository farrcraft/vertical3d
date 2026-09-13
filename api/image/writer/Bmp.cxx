/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bmp.h"

#include <api/image/BmpHeader.h>

#include <cstring>
#include <fstream>
#include <string>

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
    const uint32_t shades = grey ? 256u : 0u;

    std::fstream file;
    file.open(static_cast<std::string>(filename).c_str(), std::fstream::out | std::fstream::binary);

    if (file.fail())
        return false;

    bmp_file_header fheader;
    memset(&fheader, 0, sizeof(bmp_file_header));

    fheader.type_ = 19778;
    fheader.offset_ = sizeof(bmp_file_header) + sizeof(bmp_info_header) + shades * sizeof(bmp_rgb_quad);
    // the total file size, filled in below once the padded data length is known -
    // sizeof(img->data()) was the size of the pointer
    fheader.size_ = fheader.offset_;

    bmp_info_header iheader;
    memset(&iheader, 0, sizeof(bmp_info_header));

    iheader.size_ = sizeof(bmp_info_header);
    iheader.width_ = img->width();
    iheader.height_ = img->height();
    iheader.bits_ = img->bpp();
    iheader.compression_ = 0;
    // a reader sizes the table from the bit depth rather than from these, but a file that
    // says how many of its colours it uses is the one a reader outside this tree expects
    iheader.used_ = shades;
    iheader.important_ = shades;

    int32_t width;
    int32_t pad;
    width = pad = iheader.width_ * (iheader.bits_ / 8);
    // adjust pad width to dword boundary alignment
    while (pad % 4 != 0) {
        pad++;
    }

    unsigned int channels = img->bpp() / 8;
    iheader.imageSize_ = img->width() * img->height() * channels;

    // size of the image data including the per row boundary padding
    uint64_t rows = img->height();
    uint64_t rowBytes = static_cast<uint64_t>(width);
    uint64_t size = static_cast<uint64_t>(pad) * rows;

    fheader.size_ = static_cast<uint32_t>(fheader.offset_ + size);

    // write the headers
    file.write(reinterpret_cast<char*>(&fheader), sizeof(bmp_file_header));
    file.write(reinterpret_cast<char*>(&iheader), sizeof(bmp_info_header));

    // the whole ramp, because a reader takes the table's length from the bit depth and
    // reads 1 << bits entries whatever this file says it uses
    for (uint32_t shade = 0; shade < shades; ++shade) {
        bmp_rgb_quad entry;
        entry.blue_ = entry.green_ = entry.red_ = static_cast<unsigned char>(shade);
        entry.reserved_ = 0;
        file.write(reinterpret_cast<char*>(&entry), sizeof(bmp_rgb_quad));
    }

    boost::shared_ptr<Image> image = boost::make_shared<Image>(size);
    unsigned char* data = image->data();
    unsigned char* temp = img->data();

    // each row is copied on its own, because the padding is per row and the source
    // image has none of it. Walking both buffers with a single index and a modulo test
    // ran off the end of each - past the destination by a row's worth of padding, and
    // past the source by however many bytes of padding the whole image adds up to.
    for (uint64_t row = 0; row < rows; ++row) {
        unsigned char* dest = data + row * pad;
        const unsigned char* src = temp + row * rowBytes;
        if (grey) {
            // an index is one byte and has no channel order to correct
            memcpy(dest, src, static_cast<size_t>(rowBytes));
            continue;
        }
        for (uint32_t column = 0; column < img->width(); ++column) {
            // rgb in memory, bgr on disk
            dest[column * channels + 0] = src[column * channels + 2];
            dest[column * channels + 1] = src[column * channels + 1];
            dest[column * channels + 2] = src[column * channels + 0];
            if (img->format() == v3d::image::Image::Format::RGBA) {
                dest[column * channels + 3] = src[column * channels + 3];
            }
        }
    }

    // write image data
    file.write(reinterpret_cast<char*>(data), size);

    // done reading in file
    file.close();

    return true;
}

};  // namespace v3d::image::writer
