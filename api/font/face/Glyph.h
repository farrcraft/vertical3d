/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <boost/shared_ptr.hpp>

namespace v3d::font::face {

class Face;

/**
 * A glyph named by the face it is in and its index there. Index 0 is the face's missing glyph.
 **/
struct Glyph final {
    boost::shared_ptr<Face> face;
    unsigned int index = 0;
};

};  // namespace v3d::font::face
