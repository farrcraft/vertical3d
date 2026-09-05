/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "MicroPolygonGrid.h"

#include "../../api/type/AABBox.h"

#include <boost/shared_ptr.hpp>

namespace v3d::moya {
    class RenderContext;

    class ReyesPrimitive {
     public:
            ReyesPrimitive();
            virtual ~ReyesPrimitive();

            virtual bool diceable(void) const;
            virtual v3d::type::AABBox bound(void) const;
            /**
             * Break the primitive into smaller ones and submit each back to the first pass,
             * which is what decides the bucket and the diceability of each piece. The caller
             * discards this primitive afterwards either way, so a primitive that cannot be
             * usefully split submits nothing and is dropped.
             */
            virtual void split(RenderContext & rc);
            /*
                turn a primitive into a micropolygon grid
                i think this is supposed to return 1 or more grids as necessary
                signature would be:
                bool dice(MicroPolygonGridPtr & grid);
                and it would return false until done, each time creating a new grid.
                so you'd just do:
                while (!primitive_ptr->dice(grid)) { do something with grid }
            */
            virtual bool dice(boost::shared_ptr<MicroPolygonGrid> grid, RenderContext & rc);
            virtual void diceable(bool status);

     private:
            /*
                False until the first pass has measured the primitive against the grid size.
                An unmeasured primitive is therefore split rather than diced, which routes it
                through that measurement instead of assuming it small enough to skip it.
            */
            bool diceable_ = false;
    };
};  // namespace v3d::moya
