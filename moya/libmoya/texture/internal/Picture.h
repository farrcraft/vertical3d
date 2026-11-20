/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/texture/Level.h>

#include <cstdint>
#include <string>

namespace v3d::moya::texture::internal {

/** An image read to be made into a texture. **/
struct Picture {
    Level level;
    std::uint32_t channels { 0 };

    /** Whether any channel read was held as 32-bit values. **/
    bool floats { false };

    /** Whether it was an EXR file, whose chromaticities name its space. **/
    bool exr { false };

    /** An EXR file's linear space, as output::linearIdOf() gives it. **/
    std::string interopId;
};

};  // namespace v3d::moya::texture::internal
