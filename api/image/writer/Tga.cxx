/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Tga.h"

#include <api/image/Channels.h>
#include <api/image/Image.h>
#include <api/image/Writer.h>
#include <api/log/Logger.h>

#include <cstddef>
#include <cstdint>
#include <new>
#include <limits>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace {

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
    uint32_t extension_offset_;
    uint32_t dev_dir_offset_;
    char signature_[16];
    unsigned char reserved_;
    unsigned char terminator_;
};

#pragma pack(pop)

/** TGA 2.0 fixes the footer at 26 bytes. **/
constexpr std::size_t FOOTER_BYTES = 26;
static_assert(sizeof(tga_footer) == FOOTER_BYTES);

};  // namespace

namespace v3d::image::writer {
/**
 **/
Tga::Tga(const boost::shared_ptr<v3d::log::Logger> & logger) : Writer(logger) {
}

/**
 **/
std::optional<std::string> Tga::encode(const boost::shared_ptr<Image>& img) {
    if (!img) {
        return std::nullopt;
    }

    // the header holds each side in 16 bits
    if (img->width() > std::numeric_limits<uint16_t>::max() ||
        img->height() > std::numeric_limits<uint16_t>::max()) {
        return std::nullopt;
    }

    // a side is at most 65535 pixels and a pixel at most 4 bytes, so the size is computed in
    // size_t, which holds it
    const unsigned int bytespp = img->bpp() / 8;
    const std::size_t size = static_cast<std::size_t>(img->width()) * img->height() * bytespp;
    std::vector<unsigned char> scratch;
    std::string encoded;
    try {
        scratch.resize(size);
        encoded.reserve(sizeof(tga_header) + size + sizeof(tga_footer));
    } catch (const std::bad_alloc &) {
        logger()->get()->error("a {} by {} picture is too large to encode as a TGA", img->width(), img->height());
        return std::nullopt;
    }

    tga_header fheader;
    memset(&fheader, 0, sizeof(tga_header));

    fheader.width_ = static_cast<uint16_t>(img->width());
    fheader.height_ = static_cast<uint16_t>(img->height());
    fheader.bpp_ = img->bpp();
    const bool grey = img->format() == Image::Format::Grey;
    fheader.type_ = grey ? 3 : 2;
    // bit 5 of the descriptor marks a top-left origin. The rows go out top down, the order
    // Image holds them in, so the bit is set. A reader treats a clear bit as bottom up and
    // flips the picture
    fheader.descriptor_ = 0x20;

    encoded.append(reinterpret_cast<const char*>(&fheader), sizeof(fheader));

    const unsigned char* data = img->data();
    if (grey) {
        // one channel is one channel in either order, so there is nothing to swap
        memcpy(scratch.data(), data, size);
    } else {
        swapRedBlue(data, scratch.data(), {.pixels = size / bytespp, .channels = bytespp});
    }

    encoded.append(reinterpret_cast<const char*>(scratch.data()), size);

    tga_footer footer;
    memset(&footer, 0, sizeof(tga_footer));

    // The signature fills the field exactly, with no terminator. It is 16 bytes of a fixed
    // size record, not a C string.
    memcpy(footer.signature_, "TRUEVISION-XFILE", sizeof(footer.signature_));
    footer.reserved_ = '.';
    encoded.append(reinterpret_cast<const char*>(&footer), sizeof(footer));
    return encoded;
}

};  // namespace v3d::image::writer
