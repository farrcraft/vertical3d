/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "ReyesPrimitive.h"

#include <boost/shared_ptr.hpp>

#include <vector>

namespace v3d::moya {
        /*
            the screen space to be rendered is broken down into a grid of tiles (buckets).
            as geometry is converted to micropolygon grids it is placed in the appropriate 
            bucket corresponding to the screen space it occupies.
            once all geometry has been processed into buckets each bucket is rendered one
            at a time starting from the upper left corner.
            all buckets are the same size, configurable at render time.
            the render context is responsible for the bucket size
        */
        class Bucket final {
            public:
                Bucket();
                ~Bucket();
                
                void addPrimitive(boost::shared_ptr<ReyesPrimitive> primitive);
                void render(void);
    
            private:
                unsigned int _left;		// left edge of this bucket in pixel coordinates
                unsigned int _top;		// top edge of this bucket in pixel coordinates
                std::vector<boost::shared_ptr<ReyesPrimitive> >	_primitives;// list of primitives in this bucket
    
                std::vector<boost::shared_ptr<MicroPolygonGrid> >	_grids;
        };
};