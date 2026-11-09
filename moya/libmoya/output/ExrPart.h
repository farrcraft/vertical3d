/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace v3d::moya::output {

/** How each value of a part is stored: 16-bit or 32-bit floats. **/
enum class Precision : std::uint8_t {
    Half,
    Float
};

/** OpenEXR's compressions, from none to the lossy DWA and HTJ2K ones. **/
enum class Compression : std::uint8_t {
    None,
    Rle,
    Zips,
    Zip,
    Piz,
    Pxr24,
    B44,
    B44a,
    Dwaa,
    Dwab,
    Htj2k256,
    Htj2k32
};

/** One part of an EXR file: some of a film's outputs, stored one way. **/
struct ExrPart {
    /** The part's name. A file of several parts names each, and no two alike. **/
    std::string name;

    /** The outputs it holds, in order: kBeauty, or the name of one of the film's channels. **/
    std::vector<std::string> outputs;

    Precision precision { Precision::Half };
    Compression compression { Compression::Zip };
};

};  // namespace v3d::moya::output
