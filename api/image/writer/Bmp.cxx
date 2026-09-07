/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bmp.h"

#include <cstring>
#include <fstream>
#include <string>

#include <boost/make_shared.hpp>

#include "../BmpHeader.h"

namespace v3d::image::writer {
/**
 **/
Bmp::Bmp(const boost::shared_ptr<v3d::log::Logger>& logger) : Writer(logger) {
}

bool Bmp::write(std::string_view filename, const boost::shared_ptr<Image>& img) {
    std::fstream file;
    file.open(static_cast<std::string>(filename).c_str(), std::fstream::out | std::fstream::binary);

    if (file.fail())
        return false;

    bmp_file_header fheader;
    memset(&fheader, 0, sizeof(bmp_file_header));

    fheader.type_ = 19778;
    fheader.offset_ = sizeof(bmp_file_header) + sizeof(bmp_info_header);
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
