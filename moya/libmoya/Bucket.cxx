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

#include <boost/make_shared.hpp>

#include "Footprint.h"
#include "FrameBuffer.h"
#include "GridShader.h"
#include "RenderContext.h"

namespace v3d::moya {

namespace {

typedef std::array<glm::vec3, 4> Corners;

/*
    Where a micropolygon is in raster space for a sample: at its time, and seen from its
    point on the lens.

    A lens point L moves the eye across the lens and keeps the plane of focus where it was.
    A point at eye depth z then moves by L (1 - z / focus) in eye x and y before it is
    projected. On the plane of focus that is nothing, and everywhere else it is the circle of
    confusion.

    A still micropolygon is cheap. The move leaves z alone, and with it the perspective
    divide, so a corner's raster position is linear in L. Each corner is projected three
    times: at the centre of the lens, and a unit along each axis of it. Every sample's corners
    are a sum of those, and the lens's four extremes bound them all.

    A moving one is placed afresh for every sample, by the primitive's motion from its reference
    end to the sample's time. Its bound is the union of where it is at a run of times across
    the shutter. The bound is grown by the furthest a corner moves between two of those times,
    so it covers a path that curves between them.
*/
class Placement {
 public:
    Placement(const glm::mat4x4 & toRaster, float radius, float focus,
        const v3d::render::offline::MovingTransform & motion, const glm::vec2 & shutter) :
        toRaster_(toRaster), radius_(radius), focus_(focus), motion_(motion), shutter_(shutter),
        fromReference_(glm::inverse(motion.reference())) {
    }

    /**
     * Places a micropolygon's eye space corners, ready for at() and bound().
     */
    void place(const Corners & eye) {
        eye_ = eye;
        if (motion_.moving()) {
            sweep();
            return;
        }
        for (unsigned int k = 0; k < 4; k++) {
            still_[k] = project(toRaster_, eye[k]);
            if (radius_ <= 0.0f) {
                continue;
            }
            across_[k] = project(toRaster_, lensed(eye[k], glm::vec2(1.0f, 0.0f))) - still_[k];
            down_[k] = project(toRaster_, lensed(eye[k], glm::vec2(0.0f, 1.0f))) - still_[k];
        }
        stillBound();
    }

    /**
     * The placed micropolygon's corners for one sample.
     */
    Corners at(const Samples::Sample & sample) const {
        if (motion_.moving()) {
            return moved(delta(sample.time), sample.lens);
        }
        if (radius_ <= 0.0f) {
            return still_;
        }
        Corners corners;
        for (unsigned int k = 0; k < 4; k++) {
            corners[k] = still_[k] + across_[k] * sample.lens.x + down_[k] * sample.lens.y;
        }
        return corners;
    }

    /**
     * A moving micropolygon's corners for one sample, given the motion to its time.
     */
    Corners at(const Samples::Sample & sample, const glm::mat4x4 & motion) const {
        return moved(motion, sample.lens);
    }

    /**
     * How the primitive has moved from its reference end by a time, in eye space.
     */
    glm::mat4x4 delta(float time) const {
        return motion_.at(time) * fromReference_;
    }

    /**
     * Whether a sample can be covered at all. For a still micropolygon, its bound is the answer.
     * A moving one is tested against the bound of the slice of the shutter the sample's time
     * falls in. That bound is where the micropolygon was at either end of the slice, grown by
     * the furthest a corner moves in one slice.
     */
    bool reaches(const Samples::Sample & sample) const {
        if (!motion_.moving()) {
            return true;
        }
        const unsigned int slice = shutterSlice(sample.time, shutter_, STEPS);
        const glm::vec2 low = glm::vec2(glm::min(stepMin_[slice], stepMin_[slice + 1])) - glm::vec2(stride_);
        const glm::vec2 high = glm::vec2(glm::max(stepMax_[slice], stepMax_[slice + 1])) + glm::vec2(stride_);
        return sample.raster.x >= low.x && sample.raster.x <= high.x &&
            sample.raster.y >= low.y && sample.raster.y <= high.y;
    }

    const glm::vec3 & min() const {
        return min_;
    }

    const glm::vec3 & max() const {
        return max_;
    }

 private:
    /**
     * An eye space point moved by a point on the unit lens.
     */
    glm::vec3 lensed(const glm::vec3 & point, const glm::vec2 & lens) const {
        if (radius_ <= 0.0f) {
            return point;
        }
        const glm::vec2 shift = lens * radius_ * (1.0f - point.z / focus_);
        return glm::vec3(point.x - shift.x, point.y - shift.y, point.z);
    }

    Corners moved(const glm::mat4x4 & delta, const glm::vec2 & lens) const {
        Corners corners;
        for (unsigned int k = 0; k < 4; k++) {
            corners[k] = project(toRaster_, lensed(glm::vec3(delta * glm::vec4(eye_[k], 1.0f)), lens));
        }
        return corners;
    }

    void grow(const Corners & corners) {
        for (const glm::vec3 & corner : corners) {
            min_ = glm::min(min_, corner);
            max_ = glm::max(max_, corner);
        }
    }

    void stillBound() {
        min_ = glm::vec3(std::numeric_limits<float>::max());
        max_ = glm::vec3(-std::numeric_limits<float>::max());
        Samples::Sample sample;
        for (const glm::vec2 & lens : extremes()) {
            sample.lens = lens;
            grow(at(sample));
            if (radius_ <= 0.0f) {
                break;
            }
        }
    }

    void sweep() {
        min_ = glm::vec3(std::numeric_limits<float>::max());
        max_ = glm::vec3(-std::numeric_limits<float>::max());
        stride_ = 0.0f;
        Corners previous;
        for (unsigned int i = 0; i <= STEPS; i++) {
            const float time = shutter_.x + (shutter_.y - shutter_.x) * static_cast<float>(i) / STEPS;
            const glm::mat4x4 moving = delta(time);
            stepMin_[i] = glm::vec3(std::numeric_limits<float>::max());
            stepMax_[i] = glm::vec3(-std::numeric_limits<float>::max());
            for (const glm::vec2 & lens : extremes()) {
                for (const glm::vec3 & corner : moved(moving, lens)) {
                    stepMin_[i] = glm::min(stepMin_[i], corner);
                    stepMax_[i] = glm::max(stepMax_[i], corner);
                }
                if (radius_ <= 0.0f) {
                    break;
                }
            }
            min_ = glm::min(min_, stepMin_[i]);
            max_ = glm::max(max_, stepMax_[i]);
            const Corners centre = moved(moving, glm::vec2(0.0f));
            if (i > 0) {
                for (unsigned int k = 0; k < 4; k++) {
                    stride_ = std::max(stride_, glm::length(glm::vec2(centre[k]) - glm::vec2(previous[k])));
                }
            }
            previous = centre;
        }
        min_ -= glm::vec3(stride_, stride_, 0.0f);
        max_ += glm::vec3(stride_, stride_, 0.0f);
    }

    /**
     * The centre of the lens and its four extremes, which bound every point on it.
     */
    static std::array<glm::vec2, 5> extremes() {
        return {
            glm::vec2(0.0f), glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, -1.0f),
            glm::vec2(1.0f, 1.0f), glm::vec2(-1.0f, 1.0f)
        };
    }

    glm::mat4x4 toRaster_;
    float radius_;
    float focus_;
    const v3d::render::offline::MovingTransform & motion_;
    glm::vec2 shutter_;
    glm::mat4x4 fromReference_;
    Corners eye_;
    Corners still_;
    Corners across_;
    Corners down_;
    glm::vec3 min_ { 0.0f };
    glm::vec3 max_ { 0.0f };

    /** How many slices a moving micropolygon's shutter is measured in. **/
    static const unsigned int STEPS = 8;
    std::array<glm::vec3, STEPS + 1> stepMin_;
    std::array<glm::vec3, STEPS + 1> stepMax_;
    float stride_ { 0.0f };
};

/*
    The motion from a moving primitive's reference end to each sample's time, worked out once per
    sample over a region of the frame. Every micropolygon of a grid whose swept bound reaches a
    sample tests it, so without this the same motion is worked out for each of them.
*/
class Motions {
 public:
    Motions(const Placement & placement, unsigned int perPixel, int left, int top, int right, int bottom) :
        placement_(placement), perPixel_(perPixel), left_(left), top_(top),
        width_(std::max(0, right - left + 1)), height_(std::max(0, bottom - top + 1)),
        values_(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) * perPixel),
        known_(values_.size(), false) {
    }

    const glm::mat4x4 & at(int column, int row, unsigned int index, float time) {
        const std::size_t slot = (static_cast<std::size_t>(row - top_) * static_cast<std::size_t>(width_) +
            static_cast<std::size_t>(column - left_)) * perPixel_ + index;
        if (!known_[slot]) {
            values_[slot] = placement_.delta(time);
            known_[slot] = true;
        }
        return values_[slot];
    }

 private:
    const Placement & placement_;
    unsigned int perPixel_;
    int left_;
    int top_;
    int width_;
    int height_;
    std::vector<glm::mat4x4> values_;
    std::vector<bool> known_;
};

/**
 * The pixels a bound touches in the frame the samples cover.
 */
std::array<int, 4> pixels(const glm::vec3 & min, const glm::vec3 & max, const Samples & samples) {
    // a side is at most largestResolution, which an int holds
    return footprint(min, max, static_cast<int>(samples.width()), static_cast<int>(samples.height()));
}

/*
    Every sample in the pixels a micropolygon's bound touches that the micropolygon covers
    takes its colour, where its depth there beats what the sample already holds.
*/
void hide(const Placement & placement, const glm::vec3 & color, Samples * samples, Motions * motions) {
    const std::array<int, 4> region = pixels(placement.min(), placement.max(), *samples);
    for (int row = region[1]; row <= region[3]; row++) {
        for (int column = region[0]; column <= region[2]; column++) {
            for (unsigned int k = 0; k < samples->perPixel(); k++) {
                Samples::Sample & sample = samples->at(static_cast<unsigned int>(column),
                    static_cast<unsigned int>(row), k);
                if (!placement.reaches(sample)) {
                    continue;
                }
                const Corners corners = motions == nullptr ? placement.at(sample) :
                    placement.at(sample, motions->at(column, row, k, sample.time));
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
void hide(MicroPolygonGrid & grid, const ReyesPrimitive & primitive, RenderContext & rc) {
    // eye space to raster is the projection and then the scale into pixels, in that
    // order - a matrix applies to what is on its right
    const glm::mat4x4 toRaster = rc.coordinateSystem("raster") * rc.coordinateSystem("screen");
    // an orthographic camera has no lens to blur through
    const float radius = rc.perspective() ? rc.sampling().lensRadius() : 0.0f;
    Placement placement(toRaster, radius, rc.sampling().focalDistance, primitive.motion(), rc.sampling().shutter);
    Samples & samples = rc.samples();

    const auto corners = [&grid](unsigned int i, unsigned int j) {
        MicroPolygon poly = grid.microPolygon(i, j);
        Corners eye;
        for (unsigned int k = 0; k < 4; k++) {
            eye[k] = poly[k].point();
        }
        return eye;
    };

    // a moving grid's motions are shared by its micropolygons, over the region all of them
    // sweep, which takes a pass to measure
    boost::shared_ptr<Motions> motions;
    if (primitive.motion().moving()) {
        glm::vec3 min(std::numeric_limits<float>::max());
        glm::vec3 max(-std::numeric_limits<float>::max());
        for (unsigned int i = 0; i + 1 < grid.size(); i++) {
            for (unsigned int j = 0; j + 1 < grid.size(); j++) {
                placement.place(corners(i, j));
                min = glm::min(min, placement.min());
                max = glm::max(max, placement.max());
            }
        }
        const std::array<int, 4> region = pixels(min, max, samples);
        const std::size_t entries = static_cast<std::size_t>(std::max(0, region[2] - region[0] + 1)) *
            static_cast<std::size_t>(std::max(0, region[3] - region[1] + 1)) * samples.perPixel();
        // a grid that sweeps much of the frame would need gigabytes of motions; past the cap each
        // micropolygon works out its own instead, which is slower and needs no memory
        if (entries <= rc.motionCache()) {
            motions = boost::make_shared<Motions>(placement, samples.perPixel(), region[0], region[1], region[2],
                region[3]);
        }
    }

    for (unsigned int i = 0; i + 1 < grid.size(); i++) {
        for (unsigned int j = 0; j + 1 < grid.size(); j++) {
            placement.place(corners(i, j));
            // the shaded colour the surface shader left on the vertex
            hide(placement, grid.microPolygon(i, j)[0].color(), &samples, motions.get());
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
                hide(*grid, *prim, rc);
            }
        } else {
            // split the primitive into smaller primitives, which may be diceable. The
            // splitter feeds each piece back through the first pass. That pass buckets the
            // piece and decides whether it is diceable in turn, so the original is finished
            // with either way
            prim->split(rc);
            primitives_.erase(primitives_.begin() + i);
            i--;
            split = true;
        }
    }
    return split;
}

};  // namespace v3d::moya
