/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Bucket.h"

#include <vector>
//#include <boost/multi_array.hpp>

namespace v3d::moya {
    /*
        a framebuffer provides a stack of image planes.
        each image plane is a 2d grid of float values. the width and height of
        the grid corresponds to the image or screen dimensions.
        each plane in the buffer represents a color channel (separate planes for
        r, g, b, a or a single monochrome/bitmap plane) or some other float
        data associated with a pixel coordinate (such as a z-depth).
    
        color values are represented as floating point numbers in the range [0.0 .. 1.0] 
        instead of unsigned integers so the same structure can be used for the z buffer.
    
        the number of planes is determined in part by calls to RiDisplay()
    */
    class FrameBuffer {
        public:
            FrameBuffer(unsigned int bucketSize[2], unsigned int imageSize[2]);
            ~FrameBuffer();

            unsigned int * bucketSize(void) const;
            unsigned int * imageSize(void) const;
            void addPrimitive(const boost::shared_ptr<ReyesPrimitive> & primitive, const v3d::type::AABBox & bound);
            void render(void);
    
            //typedef boost::multi_array<Bucket, 2> BucketGrid;
            typedef std::vector< std::vector<Bucket> > BucketGrid;

        protected:
            void allocate(void);

        private:
            // plane_t plane(boost::extents[640][480]);
            //typedef boost::multi_array<float, 2> plane_t;
            typedef std::vector< std::vector<float> > plane_t;

            BucketGrid				_buckets;
            std::vector<plane_t>	_planes;
            unsigned int			_bucketSize[2];
            unsigned int			_imageSize[2];
    };

};