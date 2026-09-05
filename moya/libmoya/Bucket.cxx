/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bucket.h"

#include <iostream>
#include <vector>

namespace v3d::moya {
    Bucket::Bucket() {
    }

    Bucket::~Bucket() {
    }

    void Bucket::addPrimitive(boost::shared_ptr<ReyesPrimitive> primitive) {
        primitives_.push_back(primitive);
    }

    size_t Bucket::primitiveCount(void) const {
        return primitives_.size();
    }

    void Bucket::render(RenderContext & rc) {
        // iterate over each primitive in the bucket
        // splitting resubmits pieces through the first pass, which may append to this same
        // bucket, so the loop reads the size each time around rather than caching it
        for (size_t i = 0; i < primitives_.size(); i++) {
            // a copy, because the entry it came from is erased below while it is still in use
            boost::shared_ptr<ReyesPrimitive> prim = primitives_[i];
            // if primitive can be diced
            if (prim->diceable()) {
                // dice primitive into grid of micropolygons
                boost::shared_ptr<MicroPolygonGrid> grid;
                while (prim->dice(grid, rc)) {
                    // compute normals and tangent vectors for micropolygons in grid
                    // shade micropolygons in grid
                    // break grid into micropolygons
                    // for each micropolygon
                        // bound micropolygon in eye space
                        // if micropolygon outside hither-yon range, cull it
                        // convert micropolygon to screen space
                        // if micropolygon overlaps other buckets
                            // put micropolygon in each bucket it overlaps
                            // (even though micropolygons are less than 1 pixel, the grid it is in might overlap another bucket)
                        // for each sample point inside the screen space bound
                            // if sample point is outside micropolygon
                                // calculate z of micropolygon at sample point by interpolation
                                // if z at sample point less than z already in buffer
                                    // replace sample in buffer with this sample
                }
            } else {
                // split primitive into smaller (possibly diceable) primitives
                // the splitter feeds each piece back through the first pass, which is what
                // buckets it and decides whether it is diceable in turn, so the original is
                // finished with either way
                prim->split(rc);
                primitives_.erase(primitives_.begin() + i);
                i--;
            }
        }
        // filter visible sample hits to produce pixels
        // output pixels
    }

};  // namespace v3d::moya
