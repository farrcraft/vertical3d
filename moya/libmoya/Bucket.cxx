/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bucket.h"

#include <iostream>

namespace v3d::moya {
    Bucket::Bucket() {
    }

    Bucket::~Bucket() {
    }

    void Bucket::addPrimitive(boost::shared_ptr<ReyesPrimitive> primitive) {
        _primitives.push_back(primitive);
    }

    void Bucket::render(void) {
        // iterate over each primitive in the bucket
        std::vector<boost::shared_ptr<ReyesPrimitive> >::iterator it = _primitives.begin();
        boost::shared_ptr<ReyesPrimitive> prim;
        for (unsigned int i = 0; i < _primitives.size(); i++) {
            prim = _primitives[i];
            // if primitive can be diced
            if (prim->diceable()) {
                // dice primitive into grid of micropolygons
                boost::shared_ptr<MicroPolygonGrid> grid;
                while (prim->dice(grid)) {
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
                prim->split();
                // put new primitives at head of unread portion of model file
                // (go back and start with the first pass for each new split primitive)
                // the splitter is responsible for feeding new polygons back in
                // the old original polygon can be discarded now
                // delete prim;
                _primitives.erase(_primitives.begin() + i);
                i--;
            }
        }
        // filter visible sample hits to produce pixels
        // output pixels
    }

};  // namespace v3d::moya
