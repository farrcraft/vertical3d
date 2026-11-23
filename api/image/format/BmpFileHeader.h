/*
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <stdint.h>

/**
 * What the reader and the writer of a format share: the bmp headers and palette entry, and the
 * error manager libjpeg is given.
 **/
namespace v3d::image::format {

// the type_ of every bmp: the bytes "BM", read as a little endian word
constexpr uint16_t BMP_SIGNATURE = 19778;

#pragma pack(push, 2)

/**
 * The file header at the start of every bmp, as the file stores it.
 **/
struct bmp_file_header {
    uint16_t type_;
    uint32_t size_;
    uint16_t reserved1_;
    uint16_t reserved2_;
    uint32_t offset_;
};

#pragma pack(pop)

};  // namespace v3d::image::format
