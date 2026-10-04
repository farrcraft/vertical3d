/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bucket.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "FrameBuffer.h"
#include "GridShader.h"
#include "RenderContext.h"

namespace v3d::moya {

namespace {

/*
    A micropolygon is bounded in raster space, and every sample in the pixels the bound
    touches that the micropolygon covers takes its colour, where its depth there beats
    what the sample already holds.
*/
void hide(const std::array<glm::vec3, 4> & corners, const glm::vec3 & color, Samples * samples) {
    glm::vec3 min = corners[0];
    glm::vec3 max = corners[0];
    for (unsigned int k = 1; k < 4; k++) {
        min = glm::min(min, corners[k]);
        max = glm::max(max, corners[k]);
    }

    // a sample may be anywhere in its pixel, so every pixel the bound touches
    const int left = std::max(0, static_cast<int>(std::floor(min.x)));
    const int right = std::min(static_cast<int>(samples->width()) - 1, static_cast<int>(std::floor(max.x)));
    const int top = std::max(0, static_cast<int>(std::floor(min.y)));
    const int bottom = std::min(static_cast<int>(samples->height()) - 1, static_cast<int>(std::floor(max.y)));

    for (int row = top; row <= bottom; row++) {
        for (int column = left; column <= right; column++) {
            for (unsigned int k = 0; k < samples->perPixel(); k++) {
                Samples::Sample & sample = samples->at(static_cast<unsigned int>(column),
                    static_cast<unsigned int>(row), k);
                float depth = 0.0f;
                if (!covers(corners, sample.raster, &depth) || (sample.hit && depth >= sample.depth)) {
                    continue;
                }
                sample.colour = color;
                sample.opacity = glm::vec3(1.0f);
                sample.depth = depth;
                sample.hit = true;
            }
        }
    }
}

/*
    Every micropolygon of a grid into the frame's samples. The film filters the samples
    into pixels once every bucket is done.
*/
void hide(MicroPolygonGrid & grid, RenderContext & rc) {
    // eye space to raster is the projection and then the scale into pixels, in that
    // order - a matrix applies to what is on its right
    glm::mat4x4 toRaster = rc.coordinateSystem("raster") * rc.coordinateSystem("screen");

    for (unsigned int i = 0; i + 1 < grid.size(); i++) {
        for (unsigned int j = 0; j + 1 < grid.size(); j++) {
            MicroPolygon poly = grid.microPolygon(i, j);
            std::array<glm::vec3, 4> corners;
            for (unsigned int k = 0; k < 4; k++) {
                corners[k] = project(toRaster, poly[k].point());
            }
            // the shaded colour, which is what the surface shader left on the vertex
            hide(corners, poly[0].color(), &rc.samples());
        }
    }
}

};  // namespace

Bucket::Bucket() {
}

Bucket::~Bucket() {
}

void Bucket::addPrimitive(const boost::shared_ptr<ReyesPrimitive>& primitive) {
    primitives_.push_back(primitive);
}

size_t Bucket::primitiveCount(void) const {
    return primitives_.size();
}

bool Bucket::render(RenderContext & rc) {
    bool split = false;
    // iterate over each primitive in the bucket
    // splitting resubmits pieces through the first pass, which may append to this same
    // bucket, so the loop reads the size each time around rather than caching it
    for (size_t i = 0; i < primitives_.size(); i++) {
        // a copy, because the entry it came from is erased below while it is still in use
        boost::shared_ptr<ReyesPrimitive> prim = primitives_[i];
        if (prim->diceable()) {
            boost::shared_ptr<MicroPolygonGrid> grid;
            while (prim->dice(grid, rc)) {
                // dicing carries the primitive's own colour onto the grid as Cs, and the
                // surface shader runs over every vertex of it at once and leaves Ci there
                rc.shader().shade(prim->shading(), grid.get());
                hide(*grid, rc);
            }
        } else {
            // split primitive into smaller (possibly diceable) primitives
            // the splitter feeds each piece back through the first pass, which is what
            // buckets it and decides whether it is diceable in turn, so the original is
            // finished with either way
            prim->split(rc);
            primitives_.erase(primitives_.begin() + i);
            i--;
            split = true;
        }
    }
    return split;
}

};  // namespace v3d::moya
