/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>

namespace v3d::image::writer::internal {

#pragma pack(push, 1)

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

};  // namespace v3d::image::writer::internal
