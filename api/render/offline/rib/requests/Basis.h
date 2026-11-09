/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <array>

namespace v3d::render::offline::rib::requests {

/**
 * RiBasis: the matrices cubic patches and curves are evaluated through, and how many control
 * points each segment moves on, in u and in v. A curve runs in v.
 *
 * A matrix is in RI's order, row by row, so a point at t is [t^3 t^2 t 1] times the matrix
 * times the segment's four control points. The default is Bezier in both, stepping by 3.
 **/
struct Basis {
    using Matrix = std::array<float, 16>;

    static constexpr Matrix BEZIER{ -1, 3, -3, 1, 3, -6, 3, 0, -3, 3, 0, 0, 1, 0, 0, 0 };
    static constexpr unsigned int BEZIER_STEP = 3;

    Matrix u = BEZIER;
    unsigned int ustep = BEZIER_STEP;
    Matrix v = BEZIER;
    unsigned int vstep = BEZIER_STEP;
};

};  // namespace v3d::render::offline::rib::requests
