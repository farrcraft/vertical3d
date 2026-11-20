/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/output/ExrPart.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::moya::output {

/** The format of an output's file. **/
enum class Format : std::uint8_t {
    Exr,
    /** A deep scanline EXR file of each pixel's samples in depth. **/
    DeepExr,
    Png,
    /** A picture shown while rendering, which moya has no window for. **/
    Framebuffer
};

/** A file a scene's output asks for, and the channels it holds beside the beauty. **/
struct Output {
    std::string file;
    Format format { Format::Exr };
    /**
     * The film channels' names, in the order the output names them. A Cryptomatte layer is the
     * film channels of its ranks.
     **/
    std::vector<std::string> channels;

    /** The film channels written as 32-bit floats whatever the precision: a layer's ranks. **/
    std::vector<std::string> floats;

    /** The names of the Cryptomatte layers it holds, whose manifests its header carries. **/
    std::vector<std::string> cryptomattes;
    Precision precision { Precision::Half };
    Compression compression { Compression::Zip };

    /**
     * The colour the beauty is composited over when it is written, in the colour space named
     * beside it, or none. The beauty's alpha becomes one where it is.
     **/
    std::optional<glm::vec3> background;
    std::string backgroundSpace;
};

};  // namespace v3d::moya::output
