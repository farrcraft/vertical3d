/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/kernel/geometry/Motion.h>

#include <cstdint>
#include <functional>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

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
};

/**
 * The bounds of a primitive over a time range, from begin to end. The builder asks for each
 * primitive's over each range it splits time into.
 **/
using Sweep = std::function<LinearBox(std::uint32_t primitive, float begin, float end)>;

/**
 * The bounds over a time range of triangles that move between samples. Each triangle's three
 * points are interpolated linearly between the samples of the mesh, as kernel::geometry::pointAt()
 * reads them, so the bounds are exact at every time and hold between.
 *
 * @param held where the mesh's points are, three per triangle in each sample
 * @param points every mesh's points
 * @param times every mesh's sample times
 **/
Sweep triangleSweep(const kernel::geometry::PointSamples & held, const std::vector<glm::vec3> & points, const std::vector<float> & times);

/**
 * The bounds over a time range of a box moved by a transformation that is interpolated between
 * keys, as kernel::geometry::transformAt() interpolates it. Between two keys, each corner of the
 * box travels a curve, along a line while the rotation holds and around one when it turns. The
 * bounds come from the times at which each coordinate of each corner turns back, so a corner
 * that swings outside the boxes at the range's ends stays inside.
 **/
LinearBox sweepBox(const Box & box, const std::vector<kernel::geometry::Key> & keys, float begin, float end);

/** A box moved by a transformation: the box of its eight corners moved. Empty for an empty box. **/
Box moveBox(const Box & box, const glm::mat4 & transform);

/**
 * A box that moves linearly over a time range, moved by a transformation that holds still. The
 * corners of the moved box at the two ends bound it between them, as each coordinate's least is a
 * minimum of lines and its greatest a maximum of them.
 **/
LinearBox moveBox(const LinearBox & box, const glm::mat4 & transform);

};  // namespace v3d::moya::accel
