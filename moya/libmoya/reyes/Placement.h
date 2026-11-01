/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/MovingTransform.h>

#include <array>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Samples.h"

namespace v3d::moya::reyes {

/**
 * Where a micropolygon is in raster space for a sample: at the sample's time, and seen from
 * the sample's point on the lens.
 *
 * Let L be the lens point scaled by the lens radius. L moves the eye across the lens and
 * leaves the plane of focus in place. A point at eye depth z then moves by -L (1 - z / focus)
 * in eye x and y before it is projected. That is zero on the plane of focus, and elsewhere it
 * is the circle of confusion.
 *
 * A still micropolygon is placed once. The lens move leaves z, and so the perspective divide,
 * unchanged, so a corner's raster position is linear in L. Each corner is projected three
 * times: at the centre of the lens, and one unit along each axis of it. Every sample's corners
 * are a weighted sum of those, and the four corners of the lens square bound them all.
 *
 * A moving micropolygon is placed again for every sample, by the primitive's motion from its
 * reference end to the sample's time. Its bound is the union of its bounds at STEPS + 1 evenly
 * spaced times across the shutter. The bound is grown by the furthest a corner moves between
 * two neighbouring times, so it also covers a path that curves between them.
 */
class Placement final {
 public:
    /** A micropolygon's four corners. **/
    typedef std::array<glm::vec3, 4> Corners;

    /** The lens a micropolygon is seen through: its radius, and the distance it focuses at. **/
    struct Lens {
        float radius = 0.0F;
        float focus = 0.0F;
    };

    Placement(const glm::mat4x4 & toRaster, const Lens & lens,
        const v3d::render::offline::MovingTransform & motion, const glm::vec2 & shutter);

    /**
     * Places a micropolygon's eye space corners, ready for at(), reaches(), min() and max().
     */
    void place(const Corners & eye);

    /**
     * The placed micropolygon's corners for one sample.
     */
    Corners at(const Samples::Sample & sample) const;

    /**
     * A moving micropolygon's corners for one sample, given the motion to its time.
     */
    Corners at(const Samples::Sample & sample, const glm::mat4x4 & motion) const;

    /**
     * How the primitive has moved from its reference end by a time, in eye space.
     */
    glm::mat4x4 delta(float time) const;

    /**
     * Whether a sample can be covered at all. A still micropolygon always returns true, because
     * hide() visits only the samples inside its bound. A moving one is tested against the bound
     * of the slice of the shutter the sample's time falls in. That bound is where the
     * micropolygon was at either end of the slice, grown by the furthest a corner moves in one
     * slice.
     */
    bool reaches(const Samples::Sample & sample) const;

    const glm::vec3 & min() const;

    const glm::vec3 & max() const;

 private:
    /**
     * An eye space point moved by a point on the unit lens.
     */
    glm::vec3 lensed(const glm::vec3 & point, const glm::vec2 & lens) const;

    Corners moved(const glm::mat4x4 & delta, const glm::vec2 & lens) const;

    void grow(const Corners & corners);

    void stillBound();

    void sweep();

    /**
     * The centre of the lens and its four extremes, which bound every point on it.
     */
    static constexpr std::array EXTREMES{
        glm::vec2(0.0F), glm::vec2(-1.0F, -1.0F), glm::vec2(1.0F, -1.0F),
        glm::vec2(1.0F, 1.0F), glm::vec2(-1.0F, 1.0F)
    };

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
    glm::vec3 min_ { 0.0F };
    glm::vec3 max_ { 0.0F };

    /** How many slices a moving micropolygon's shutter is measured in. **/
    static const unsigned int STEPS = 8;
    std::array<glm::vec3, STEPS + 1> stepMin_;
    std::array<glm::vec3, STEPS + 1> stepMax_;
    float stride_ { 0.0F };
};

};  // namespace v3d::moya::reyes
