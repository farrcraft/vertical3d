/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::moya::output {

/**
 * An image read from an EXR file: its texels' red, green and blue, row after row from its top, and
 * what its header says of them.
 *
 * - interopId is the Color Interop Forum ID of the linear space its chromaticities name, linear
 *   Rec. 709 when it has none, or empty for chromaticities of any other space.
 * - cube is whether its envmap attribute names a cube map.
 **/
struct ExrImage {
    std::uint32_t width { 0 };
    std::uint32_t height { 0 };
    std::vector<glm::vec3> texels;
    std::string interopId;
    bool cube { false };
};

/**
 * Reads the R, G and B channels of an EXR file's first part whole, as floats, its data window
 * from its top left. A channel the file lacks reads as zero, and a file of Y alone as grey.
 *
 * @param path the file, as UTF-8
 * @throws std::exception, from OpenEXR, when the file cannot be read
 **/
ExrImage readExr(const std::string & path);

};  // namespace v3d::moya::output
