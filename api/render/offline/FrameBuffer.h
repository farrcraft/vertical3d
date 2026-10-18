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
 * height are the image's. A plane holds one colour channel or some other per-pixel float,
 * such as a depth or a coverage. Colour is stored as float too, so one structure holds
 * both.
 **/
class FrameBuffer final {
 public:
    /**
     * Every plane starts at zero. A plane where zero is a real value must be clear()ed to
     * its starting value. For a depth plane, zero is the near plane, not an absence.
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

    /** A plane, and the value every pixel of it is set to. **/
    struct Fill {
        unsigned int plane = 0;
        float value = 0.0F;
    };

    /**
     * Fill a whole plane with one value.
     **/
    void clear(const Fill & fill);

    /**
     * The leading planes as an image, row 0 at the top.
     *
     * The plane count is not the channel count. A renderer's own planes sit after the
     * image channels and are not part of the picture. A depth written into alpha gives
     * a wrong image that looks like a shading fault.
     *
     * A value outside [0, 1] is clamped, because the conversion to a byte alone would wrap
     * and turn a bright pixel dark. A NaN is black.
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
