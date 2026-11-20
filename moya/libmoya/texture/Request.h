/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

namespace v3d::moya::texture {

/**
 * An image a material's program reads, as its image node names it.
 *
 * - file is the image's path as the node gives it, which may hold the token `<UDIM>`.
 * - colourSpace is the space a colour image is read in, a name or role in the colour
 *   configuration, or empty for the image's own.
 * - colour is whether the node reads a colour. An image that is not is data, never converted.
 **/
struct Request {
    std::string file;
    std::string colourSpace;
    bool colour { true };

    bool operator==(const Request &) const = default;
};

};  // namespace v3d::moya::texture
