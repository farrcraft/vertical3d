/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Tga.h"

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

namespace v3d::image::reader {
/**
 **/
Tga::Tga(const boost::shared_ptr<v3d::log::Logger>& logger) : Reader(logger) {
}

/**
 **/
boost::shared_ptr<Image> Tga::read(const unsigned char* data, std::size_t size) {
    // 2 = uncompressed rgb
    // 3 = uncompressed b&w
    const unsigned char rgbTGAheader[12] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // Uncompressed TGA Header
    const unsigned char bwTGAheader[12] = { 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // Uncompressed TGA Header

    boost::shared_ptr<Image> empty_ptr;
    if (data == nullptr) {
        return empty_ptr;
    }

    const std::size_t headerSize = sizeof(rgbTGAheader) + 6;
    if (size < headerSize) {
        return empty_ptr;
    }

    if (memcmp(rgbTGAheader, data, sizeof(rgbTGAheader)) != 0 && memcmp(bwTGAheader, data, sizeof(bwTGAheader)) != 0) {
        return empty_ptr;
    }

    // the six useful bytes after the signature
    const unsigned char* header = data + sizeof(rgbTGAheader);

    unsigned int width = header[1] * 256 + header[0];  // Determine The TGA Width (highbyte*256+lowbyte)
    unsigned int height = header[3] * 256 + header[2];  // Determine The TGA Height (highbyte*256+lowbyte)
    unsigned int bpp = header[4];  // Grab The TGA's Bits Per Pixel (24 or 32)

    if (width == 0 ||  // Is The Width Zero
        height == 0 ||  // Is The Height Zero
        (header[4] != 24 && header[4] != 32 && header[4] != 8)) {  // 8/24/32 bit?
        return empty_ptr;
    }

    const unsigned int bytespp = bpp / 8;
    const std::size_t pixels = static_cast<std::size_t>(width) * height * bytespp;
    if (size - headerSize < pixels) {
        return empty_ptr;
    }

    boost::shared_ptr<Image> img(new Image(width, height, static_cast<uint8_t>(bpp)));
    unsigned char* out = img->data();
    memcpy(out, data + headerSize, pixels);

    if (bytespp >= 3) {
        // the bound is the last byte read rather than the first, so that a whole pixel is
        // what the loop is proved to stay inside
        for (std::size_t i = 0; i + 2 < pixels; i += bytespp) {
            // Swaps The 1st And 3rd Bytes ('R'ed and 'B'lue)
            const unsigned char temp = out[i];
            out[i] = out[i + 2];
            out[i + 2] = temp;
        }
    }

    // bit 5 of the image descriptor is the vertical origin, and it is clear far more
    // often than it is set - the file then holds its rows bottom up. Every other reader
    // here yields a top down image, and so does every consumer of one, so flip rather
    // than leave the orientation up to whoever wrote the file
    if ((header[5] & 0x20) == 0) {
        const unsigned int stride = width * bytespp;
        std::vector<unsigned char> row(stride);
        for (unsigned int i = 0; i < height / 2; ++i) {
            unsigned char* top = out + static_cast<std::size_t>(i) * stride;
            unsigned char* bottom = out + static_cast<std::size_t>(height - 1 - i) * stride;
            memcpy(row.data(), top, stride);
            memcpy(top, bottom, stride);
            memcpy(bottom, row.data(), stride);
        }
    }

    return img;
}

};  // namespace v3d::image::reader
