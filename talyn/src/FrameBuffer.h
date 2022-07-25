/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "../../api/image/Image.h"

#include <boost/shared_ptr.hpp>


namespace Talyn {
    /**
     *	a framebuffer provides a stack of image planes.
     *	each image plane is a 2d grid of float values. the width and height of
     *	the grid corresponds to the image or screen dimensions.
     *	each plane in the buffer represents a color channel (separate planes for
     *	r, g, b, a or a single monochrome/bitmap plane) or some other float
     *	data associated with a pixel coordinate (such as a z-depth).
     *
     *	color values are represented as floating point numbers in the range [0.0 .. 1.0] 
     *	instead of unsigned integers so the same structure can be used for the z buffer.
     *
     *	the number of planes is determined in part by calls to RiDisplay()
     **/
    class FrameBuffer {
     public:
        FrameBuffer(unsigned int width, unsigned int height, unsigned int depth);
        ~FrameBuffer();

        /**
         * Render the framebuffer contents into an image object
         */
        boost::shared_ptr<v3d::image::Image> render();

     private:
        typedef std::vector< std::vector<float> > plane_t;

        std::vector<plane_t> planes_;
        unsigned int width_;
        unsigned int height_;
    };

};  // end namespace Talyn
