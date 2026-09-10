/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <tetris/src/Tetrad.h>

#include <string>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * An L, which is the same shape as none of its own rotations - so a turn that did
 * nothing, or turned the wrong way, shows up.
 **/
Tetrad::ShapeInfo el() {
    Tetrad::ShapeInfo shape;
    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            shape.layout_[i][j] = 0;
        }
    }
    shape.layout_[0][0] = 1;
    shape.layout_[1][0] = 1;
    shape.layout_[2][0] = 1;
    shape.layout_[2][1] = 1;
    shape.color_ = "orange";
    return shape;
}

bool same(const Tetrad::ShapeInfo & a, const Tetrad::ShapeInfo & b) {
    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            if (a.layout_[i][j] != b.layout_[i][j]) {
                return false;
            }
        }
    }
    return true;
}

};  // namespace

BOOST_AUTO_TEST_CASE(tetrad_rotation_test) {
    const Tetrad::ShapeInfo start = el();

    Tetrad piece(start);
    piece.rotate(Tetrad::CLOCKWISE);
    BOOST_CHECK_EQUAL(same(piece.shape(), start), false);
    BOOST_CHECK_EQUAL(piece.orientation(), 1u);

    // four turns the same way come back to where they started
    piece.rotate(Tetrad::CLOCKWISE);
    piece.rotate(Tetrad::CLOCKWISE);
    piece.rotate(Tetrad::CLOCKWISE);
    BOOST_CHECK_EQUAL(same(piece.shape(), start), true);
    BOOST_CHECK_EQUAL(piece.orientation(), 0u);
}

BOOST_AUTO_TEST_CASE(tetrad_counterclockwise_test) {
    const Tetrad::ShapeInfo start = el();

    // the two directions have to be each other's inverse, which they were not when the
    // counter-clockwise branch copied the layout unchanged
    Tetrad piece(start);
    piece.rotate(Tetrad::COUNTERCLOCKWISE);
    BOOST_CHECK_EQUAL(same(piece.shape(), start), false);
    BOOST_CHECK_EQUAL(piece.orientation(), 3u);

    piece.rotate(Tetrad::CLOCKWISE);
    BOOST_CHECK_EQUAL(same(piece.shape(), start), true);
    BOOST_CHECK_EQUAL(piece.orientation(), 0u);
}

BOOST_AUTO_TEST_CASE(tetrad_normalize_test) {
    Tetrad::ShapeInfo shape = el();
    // push it away from the corner
    Tetrad::ShapeInfo moved = shape;
    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            moved.layout_[i][j] = 0;
        }
    }
    moved.layout_[1][1] = 1;
    moved.layout_[2][1] = 1;

    Tetrad::normalize(&moved);
    BOOST_CHECK_EQUAL(moved.layout_[0][0], 1);
    BOOST_CHECK_EQUAL(moved.layout_[1][0], 1);
    BOOST_CHECK_EQUAL(moved.layout_[1][1], 0);

    // a turn leaves the result normalised too, so a shape never drifts out of the corner
    Tetrad piece(shape);
    piece.rotate(Tetrad::CLOCKWISE);
    Tetrad::ShapeInfo turned = piece.shape();
    Tetrad::ShapeInfo again = turned;
    Tetrad::normalize(&again);
    BOOST_CHECK_EQUAL(same(turned, again), true);
}

BOOST_AUTO_TEST_CASE(tetrad_assignment_test) {
    Tetrad piece(el());
    piece.position(Tetrad::PositionType(4, 7));

    // assignment has to carry the position, which the copy constructor beside it does
    Tetrad copy;
    copy = piece;
    BOOST_CHECK_EQUAL(copy.position().first, 4u);
    BOOST_CHECK_EQUAL(copy.position().second, 7u);
    BOOST_CHECK_EQUAL(copy.initialized(), true);
    BOOST_CHECK_EQUAL(same(copy.shape(), piece.shape()), true);
}

BOOST_AUTO_TEST_CASE(tetrad_extent_test) {
    // the L is three tall and two wide, so an extent that ignored the minimum occupied row
    // and column would report three for both
    Tetrad piece(el());
    BOOST_CHECK_EQUAL(piece.width(), 2u);
    BOOST_CHECK_EQUAL(piece.height(), 3u);

    BOOST_CHECK_EQUAL(Tetrad().width(), 0u);
}
