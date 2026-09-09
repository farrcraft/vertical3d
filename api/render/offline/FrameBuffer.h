/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Image.h>

#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::offline {
/**
 * A stack of image planes. Each plane is a 2d grid of float values whose width and
 * height are the image's, holding one colour channel or some other float associated
 * with a pixel - a depth, a coverage. Colour is float rather than an unsigned integer
 * so that the same structure serves both.
 **/
class FrameBuffer final {
 public:
    /**
     * Every plane starts at zero. A plane whose zero means something - a depth, where
     * it is the near plane rather than an absence - has to be clear()ed to what it
     * should start at.
     **/
    FrameBuffer(unsigned int width, unsigned int height, unsigned int planes);

    unsigned int width() const;
    unsigned int height() const;
    unsigned int planes() const;

    /**
     * @param plane which plane
     * @param column the pixel's x, from the left
     * @param row the pixel's y, from the top
     **/
    float value(unsigned int plane, unsigned int column, unsigned int row) const;
    void value(unsigned int plane, unsigned int column, unsigned int row, float v);

    /**
     * Fill a whole plane with one value.
     **/
    void clear(unsigned int plane, float v);

    /**
     * The leading planes as an image, row 0 at the top.
     *
     * The plane count is not the channel count: a renderer's own planes sit after the
     * image channels and are not part of the picture, and a depth written into alpha
     * is a wrong image that looks like a shading fault.
     *
     * A value outside [0, 1] saturates rather than wrapping - the cast alone turns a
     * bright pixel dark.
     *
     * @param channels how many leading planes are image channels, 3 or 4
     **/
    boost::shared_ptr<v3d::image::Image> image(unsigned int channels) const;

 private:
    typedef std::vector< std::vector<float> > plane_t;

    std::vector<plane_t> planes_;
    unsigned int width_;
    unsigned int height_;
};

};  // namespace v3d::render::offline
