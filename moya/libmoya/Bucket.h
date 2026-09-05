/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "ReyesPrimitive.h"

#include <boost/shared_ptr.hpp>

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
            /**
             * Render every primitive in the bucket: dice it into micropolygon grids, shade
             * them and sample them into the framebuffer's planes. A primitive too large to
             * dice is split instead, and its pieces go back through the first pass, which may
             * add them to this bucket or another one.
             * @return whether anything was split, and so whether the sweep has to come round
             *         again for pieces that landed behind it
             */
            bool render(RenderContext & rc);

            size_t primitiveCount(void) const;

         private:
            unsigned int left_ = 0;  // left edge of this bucket in pixel coordinates
            unsigned int top_ = 0;  // top edge of this bucket in pixel coordinates
            std::vector<boost::shared_ptr<ReyesPrimitive> > primitives_;  // list of primitives in this bucket
        };
};  // namespace v3d::moya
