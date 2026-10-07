/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Tga.h"

#include <api/image/Channels.h>

#include <fstream>
#include <iostream>
#include <limits>
#include <cstring>
#include <string>
#include <vector>

#pragma pack(push, 1)

struct tga_header {
    unsigned char id_;
    unsigned char colormap_;  // 0=none, 1=palette
    unsigned char type_;  // 0=none, 1=indexed, 2=rgb, 3=grey, 4=rle

    uint16_t cmap_start_;
    uint16_t cmap_length_;
    unsigned char cmap_bits_;  // 15, 16, 24, 32

    uint16_t xorigin_;
    uint16_t yorigin_;
    uint16_t width_;
    uint16_t height_;
    unsigned char bpp_;  // 8, 16, 24, 32
    unsigned char descriptor_;
};

struct tga_footer {
    int64_t extension_offset_;
    int64_t dev_dir_offset_;
    char signature_[16];
    unsigned char reserved_;
    unsigned char terminator_;
};

#pragma pack(pop)

namespace v3d::image::writer {
/**
 **/
Tga::Tga(const boost::shared_ptr<v3d::log::Logger> & logger) : Writer(logger) {
}

/**
 **/
bool Tga::write(std::string_view filename, const boost::shared_ptr<Image>& img) {
    if (!img) {
        return false;
    }

    // checked before the open, which truncates: a picture the header cannot describe
    // leaves any file already at that name as it was
    if (img->width() > std::numeric_limits<uint16_t>::max() ||
        img->height() > std::numeric_limits<uint16_t>::max()) {
        return false;
    }

    std::fstream file;
    file.open(static_cast<std::string>(filename).c_str(), std::fstream::out | std::fstream::binary);
    if (file.fail()) {
        return false;
    }

    tga_header fheader;
    memset(&fheader, 0, sizeof(tga_header));

    fheader.width_ = static_cast<uint16_t>(img->width());
    fheader.height_ = static_cast<uint16_t>(img->height());
    fheader.bpp_ = img->bpp();
    const bool grey = img->format() == Image::Format::Grey;
    fheader.type_ = grey ? 3 : 2;
    // bit 5 of the descriptor is the vertical origin, and the rows below go out top down
    // because that is the order Image holds them in. Leaving it clear claims bottom up,
    // which a reader is entitled to act on by turning the picture over
    fheader.descriptor_ = 0x20;

    file.write(reinterpret_cast<char*>(&fheader), sizeof(fheader));

    unsigned int bytespp = fheader.bpp_ / 8;
    unsigned int size = fheader.width_ * fheader.height_ * bytespp;
    std::vector<unsigned char> scratch(size);
    const unsigned char* data = img->data();

    if (file.fail()) {
        file.close();
        return false;
    }

    if (grey) {
        // one channel is one channel in either order, so there is nothing to swap
        memcpy(scratch.data(), data, size);
    } else {
        swapRedBlue(data, scratch.data(), size / bytespp, bytespp);
    }

    file.write(reinterpret_cast<char*>(scratch.data()), size);

    tga_footer footer;
    memset(&footer, 0, sizeof(tga_footer));

    // The signature fills the field exactly, with no terminator - it is 16 bytes of a
    // fixed size record rather than a C string.
    memcpy(footer.signature_, "TRUEVISION-XFILE", sizeof(footer.signature_));
    footer.reserved_ = '.';
    file.write(reinterpret_cast<char*>(&footer), sizeof(footer));

    file.close();
    return true;
}

};  // namespace v3d::image::writer
