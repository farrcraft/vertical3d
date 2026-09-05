/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bucket.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "FrameBuffer.h"
#include "RenderContext.h"

namespace v3d::moya {

namespace {

/*
    One sample per pixel centre, no pixel filter: each micropolygon is bounded in
    raster space and every pixel centre the bound covers takes its colour, where its
    depth beats what the plane already holds.
*/
void hide(MicroPolygonGrid & grid, RenderContext & rc) {
    boost::shared_ptr<FrameBuffer> framebuffer = rc.framebuffer();
    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = framebuffer->planes();

    // eye space to raster is the projection and then the scale into pixels, in that
    // order - a matrix applies to what is on its right
    glm::mat4x4 toRaster = rc.coordinateSystem("raster") * rc.coordinateSystem("screen");

    const int width = static_cast<int>(planes->width());
    const int height = static_cast<int>(planes->height());

    for (unsigned int i = 0; i + 1 < grid.size(); i++) {
        for (unsigned int j = 0; j + 1 < grid.size(); j++) {
            MicroPolygon poly = grid.microPolygon(i, j);

            glm::vec3 corner = project(toRaster, poly[0].point());
            glm::vec3 min = corner;
            glm::vec3 max = corner;
            float depth = corner.z;
            for (unsigned int k = 1; k < 4; k++) {
                corner = project(toRaster, poly[k].point());
                min = glm::min(min, corner);
                max = glm::max(max, corner);
                depth += corner.z;
            }
            // a micropolygon is smaller than a pixel, so one depth for the whole of
            // it is as fine as the sampling can tell
            depth /= 4.0f;

            // a pixel is sampled at its centre, so column c is covered when the bound
            // spans c + 0.5
            int left = std::max(0, static_cast<int>(std::ceil(min.x - 0.5f)));
            int right = std::min(width - 1, static_cast<int>(std::floor(max.x - 0.5f)));
            int top = std::max(0, static_cast<int>(std::ceil(min.y - 0.5f)));
            int bottom = std::min(height - 1, static_cast<int>(std::floor(max.y - 0.5f)));

            const glm::vec3 color = poly[0].color();
            for (int row = top; row <= bottom; row++) {
                for (int column = left; column <= right; column++) {
                    unsigned int x = static_cast<unsigned int>(column);
                    unsigned int y = static_cast<unsigned int>(row);
                    if (depth >= planes->value(FrameBuffer::DEPTH, x, y)) {
                        continue;
                    }
                    planes->value(FrameBuffer::RED, x, y, color.r);
                    planes->value(FrameBuffer::GREEN, x, y, color.g);
                    planes->value(FrameBuffer::BLUE, x, y, color.b);
                    planes->value(FrameBuffer::DEPTH, x, y, depth);
                }
            }
        }
    }
}

};  // namespace

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
                // dicing carries the primitive's own colour onto the grid, which is the
                // whole of shading until RiSurface has an implementation
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
