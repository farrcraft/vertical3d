/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/geometry/primitive/Quadric.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>

#include <cstdint>
#include <functional>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::accel {

/**
 * A box over a time range: a box at its start and one at its end, with the box at a time between
 * interpolated linearly. It bounds what it holds at every time of the range.
 **/
struct LinearBox {
    Box start;
    Box end;

    /** A box that holds the same at every time. **/
    static LinearBox still(const Box & box) {
        return LinearBox { .start = box, .end = box };
    }

    bool empty() const {
        return start.empty() && end.empty();
    }

    /** Grows both ends by another's, which then still bounds what each held. **/
    void grow(const LinearBox & other) {
        start.grow(other.start);
        end.grow(other.end);
    }

    /** The box at a fraction of the range, from 0 at its start to 1 at its end. **/
    Box at(float along) const;

    /**
     * The box's surface area averaged over its range. The area is quadratic in time, so three
     * points give it exactly.
     **/
    float area() const;

    /** The box of everything it holds over the range. **/
    Box swept() const;

 private:
    /** Simpson's rule over three points: the middle one's weight, and the sum of the weights. **/
    static constexpr float kMiddle = 4.0F;
    static constexpr float kWeights = 6.0F;
};

/**
 * The bounds of a primitive over a time range, from begin to end. The builder asks for each
 * primitive's over each range it splits time into.
 **/
using Sweep = std::function<LinearBox(std::uint32_t primitive, float begin, float end)>;

/**
 * The bounds over a time range of triangles that move between samples. Each triangle's three
 * points are interpolated linearly between the samples of the mesh, as kernel::geometry::sampleAt()
 * reads them, so the bounds are exact at every time and hold between.
 *
 * @param held where the mesh's points are, three per triangle in each sample
 * @param points every mesh's points
 * @param times every mesh's sample times
 **/
Sweep triangleSweep(const kernel::geometry::Sampled & held, const std::vector<glm::vec3> & points, const std::vector<float> & times);

/**
 * The bounds over a time range of a box moved by a transformation that is interpolated between
 * keys, as kernel::geometry::transformAt() interpolates it. Between two keys, each corner of the
 * box travels a curve, along a line while the rotation holds and around one when it turns. The
 * bounds come from the times at which each coordinate of each corner turns back, so a corner
 * that swings outside the boxes at the range's ends stays inside.
 **/
LinearBox sweepBox(const Box & box, const std::vector<kernel::geometry::Key> & keys, float begin, float end);

/**
 * The bounds over a time range of points that move between samples, each a sphere or a disk of
 * half its width about its centre. The centres and the widths each move linearly between their
 * own samples, so the bounds hold at every sample of either inside the range, and so between.
 *
 * @param centres where the points' centres are, one per point in each sample
 * @param widths where the points' widths are, one per point in each sample
 **/
Sweep pointSweep(const kernel::geometry::Sampled & centres, const kernel::geometry::Sampled & widths, const std::vector<glm::vec3> & points,
    const std::vector<float> & widthValues, const std::vector<float> & times);

/**
 * The bounds over a time range of segments of curves whose control points move between samples. A
 * segment lies inside the hull of its control points, each grown by its radius. Each control
 * point's centre and radius moves linearly between the samples, so the least of them is concave
 * between two samples and the greatest convex. Bounds that hold at every sample inside the range
 * then hold between.
 *
 * @param held where the control points are, perSegment to a segment in each sample
 * @param controls every segment's control points
 **/
Sweep curveSweep(const kernel::geometry::Sampled & held, std::uint32_t perSegment, const std::vector<glm::vec4> & controls,
    const std::vector<float> & times);

/**
 * The box of a quadric whose dimensions each lie between least's and most's. It holds the whole
 * surface of revolution, whatever its sweep.
 **/
Box quadricBounds(const kernel::geometry::primitive::Quadric & least, const kernel::geometry::primitive::Quadric & most);

/**
 * The bounds over a time range of a quadric whose dimensions move between samples. Each dimension
 * moves linearly, so its least and its greatest over the range are at a sample inside it or at an
 * end, and quadricBounds() of those holds the quadric throughout. The bounds hold still.
 **/
LinearBox quadricSweep(const kernel::geometry::Sampled & held, const std::vector<kernel::geometry::primitive::Quadric> & quadrics,
    const std::vector<float> & times, float begin, float end);

/** A box moved by a transformation: the box of its eight corners moved. Empty for an empty box. **/
Box moveBox(const Box & box, const glm::mat4 & transform);

/**
 * A box that moves linearly over a time range, moved by a transformation that holds still. The
 * corners of the moved box at the two ends bound it between them, as each coordinate's least is a
 * minimum of lines and its greatest a maximum of them.
 **/
LinearBox moveBox(const LinearBox & box, const glm::mat4 & transform);

};  // namespace v3d::moya::accel
