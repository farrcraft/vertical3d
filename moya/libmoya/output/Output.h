/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/output/ExrPart.h>

#include <cstdint>
#include <string>
#include <vector>

namespace v3d::moya::output {

/** The format of an output's file. **/
enum class Format : std::uint8_t {
    Exr,
    Png,
    /** A picture shown while rendering, which moya has no window for. **/
    Framebuffer
};

/** A file a scene's output asks for, and the channels it holds beside the beauty. **/
struct Output {
    std::string file;
    Format format { Format::Exr };
    /** The film channels' names, in the order the output names them. **/
    std::vector<std::string> channels;
    Precision precision { Precision::Half };
    Compression compression { Compression::Zip };
};

};  // namespace v3d::moya::output
