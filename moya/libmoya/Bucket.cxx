/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bucket.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

#include "FrameBuffer.h"
#include "GridShader.h"
#include "RenderContext.h"

namespace v3d::moya {

namespace {

typedef std::array<glm::vec3, 4> Corners;

/*
    Where a micropolygon is in raster space for a sample's point on the lens.

    A lens point L moves the eye across the lens and keeps the plane of focus where it was,
    so a point at eye depth z moves by L (1 - z / focus) in eye x and y before it is
    projected. On the plane of focus that is nothing, and everywhere else it is the circle of
    confusion. The move leaves z, and so the perspective divide, alone, which makes a corner's
    raster position linear in L: a micropolygon is projected three times, at the centre of
    the lens and a unit along each axis of it, and every sample's corners are a sum of those.
*/
class Lens {
 public:
    Lens(const glm::mat4x4 & toRaster, float radius, float focus) :
        toRaster_(toRaster), radius_(radius), focus_(focus) {
    }

    bool pinhole() const {
        return radius_ <= 0.0f;
    }

    /**
     * Projects a micropolygon's eye space corners, ready for at().
     */
    void place(const Corners & eye) {
        for (unsigned int k = 0; k < 4; k++) {
            still_[k] = project(toRaster_, eye[k]);
            if (pinhole()) {
                continue;
            }
            const float shift = radius_ * (1.0f - eye[k].z / focus_);
            across_[k] = project(toRaster_, eye[k] - glm::vec3(shift, 0.0f, 0.0f)) - still_[k];
            down_[k] = project(toRaster_, eye[k] - glm::vec3(0.0f, shift, 0.0f)) - still_[k];
        }
    }

    /**
     * The placed micropolygon's corners for a point on the unit lens.
     */
    Corners at(const glm::vec2 & lens) const {
        if (pinhole()) {
            return still_;
        }
        Corners corners;
        for (unsigned int k = 0; k < 4; k++) {
            corners[k] = still_[k] + across_[k] * lens.x + down_[k] * lens.y;
        }
        return corners;
    }

    /**
     * The placed micropolygon's raster bound over every point on the lens, which the lens's
     * four extremes bound because the corners move linearly.
     */
    void bound(glm::vec3 * min, glm::vec3 * max) const {
        const glm::vec2 extremes[5] = {
            glm::vec2(0.0f), glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, -1.0f),
            glm::vec2(1.0f, 1.0f), glm::vec2(-1.0f, 1.0f)
        };
        const unsigned int count = pinhole() ? 1 : 5;
        *min = glm::vec3(std::numeric_limits<float>::max());
        *max = glm::vec3(-std::numeric_limits<float>::max());
        for (unsigned int e = 0; e < count; e++) {
            for (const glm::vec3 & corner : at(extremes[e])) {
                *min = glm::min(*min, corner);
                *max = glm::max(*max, corner);
            }
        }
    }

 private:
    glm::mat4x4 toRaster_;
    float radius_;
    float focus_;
    Corners still_;
    Corners across_;
    Corners down_;
};

/*
    Every sample in the pixels a micropolygon's bound touches that the micropolygon covers
    takes its colour, where its depth there beats what the sample already holds.
*/
void hide(const Lens & lens, const glm::vec3 & color, Samples * samples) {
    glm::vec3 min;
    glm::vec3 max;
    lens.bound(&min, &max);

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
                if (!covers(lens.at(sample.lens), sample.raster, &depth) || (sample.hit && depth >= sample.depth)) {
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
    const glm::mat4x4 toRaster = rc.coordinateSystem("raster") * rc.coordinateSystem("screen");
    // an orthographic camera has no lens to blur through
    const float radius = rc.perspective() ? rc.sampling().lensRadius() : 0.0f;
    Lens lens(toRaster, radius, rc.sampling().focalDistance);

    for (unsigned int i = 0; i + 1 < grid.size(); i++) {
        for (unsigned int j = 0; j + 1 < grid.size(); j++) {
            MicroPolygon poly = grid.microPolygon(i, j);
            Corners eye;
            for (unsigned int k = 0; k < 4; k++) {
                eye[k] = poly[k].point();
            }
            lens.place(eye);
            // the shaded colour, which is what the surface shader left on the vertex
            hide(lens, poly[0].color(), &rc.samples());
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
