/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string_view>

namespace v3d::font::internal {

/**
 * An SVG table's document, as the table holds it.
 **/
struct SvgDocument final {
    std::string_view text;       /**< plain, or compressed by gzip **/
    unsigned int unitsPerEm = 0; /**< the face's, which is the document's first viewport each way **/
    FT_Memory memory = nullptr;  /**< FreeType's, which inflates a compressed document **/
};

};  // namespace v3d::font::internal
