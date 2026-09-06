/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Tetrad.h"

#include <iostream>
#include <cassert>
#include <algorithm>
#include <cstring>

Tetrad::Tetrad(const ShapeInfo & shape) : orientation_(0), shape_(shape), initialized_(true) {
}

Tetrad::Tetrad() : orientation_(0), initialized_(false) {
}

Tetrad & Tetrad::operator=(const Tetrad & t) {
    orientation_ = t.orientation_;
    position_ = t.position_;
    initialized_ = t.initialized_;
    shape_ = t.shape_;
    return *this;
}

bool Tetrad::initialized() const {
    return initialized_;
}

void Tetrad::move(int dx, int dy) {
    if (dx < 0) {
        assert(position_.first + offset(OFFSET_X) > 0);
    }
    if (dy < 0) {
        assert(position_.second + offset(OFFSET_Y) > 0);
    }

    position_.first += dx;
    position_.second += dy;
}

void Tetrad::rotate(RotationDirection dir) {
    // rows run down the screen and columns across it, so a quarter turn clockwise sends the
    // cell at (row, column) to (column, 3 - row), and the other direction is its inverse
    uint8_t rotated[4][4];
    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            if (dir == CLOCKWISE) {
                rotated[j][3 - i] = shape_.layout_[i][j];
            } else {
                rotated[3 - j][i] = shape_.layout_[i][j];
            }
        }
    }
    memcpy(shape_.layout_, rotated, sizeof(rotated));

    // a turn leaves the shape wherever in the grid the arithmetic put it, and the board
    // reads the grid's corner as the tetrad's position
    normalize(&shape_);

    if (dir == CLOCKWISE) {
        orientation_ = (orientation_ + 1) % 4;
    } else {
        orientation_ = (orientation_ + 3) % 4;
    }
}

void Tetrad::normalize(ShapeInfo * shape) {
    unsigned int row = 4;
    unsigned int column = 4;
    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            if (shape->layout_[i][j] != 0) {
                row = (row < i) ? row : i;
                column = (column < j) ? column : j;
            }
        }
    }
    // an empty layout, or one already in the corner
    if (row == 4 || column == 4 || (row == 0 && column == 0)) {
        return;
    }

    uint8_t shifted[4][4] = { { 0 } };
    for (unsigned int i = 0; i + row < 4; i++) {
        for (unsigned int j = 0; j + column < 4; j++) {
            shifted[i][j] = shape->layout_[i + row][j + column];
        }
    }
    memcpy(shape->layout_, shifted, sizeof(shifted));
}

void Tetrad::position(PositionType p) {
    position_ = p;
}

Tetrad::PositionType Tetrad::position() const {
    return position_;
}

unsigned int Tetrad::orientation() const {
    return orientation_;
}

void Tetrad::orientation(unsigned int o) {
    orientation_ = o;
}

const Tetrad::ShapeInfo & Tetrad::shape() const {
    return shape_;
}

unsigned int Tetrad::offset(OffsetAxis dir) const {
    unsigned int x = 3;
    unsigned int y = 3;

    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            if (shape_.layout_[i][j] == 1) {
                x = std::min(x, j);
                y = std::min(y, i);
            }
        }
    }
    if (dir == OFFSET_X)
        return x;
    return y;
}

unsigned int Tetrad::width() const {
    unsigned int min = 4;
    unsigned int max = 0;

    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            if (shape_.layout_[i][j] == 1) {
                min = (min < j) ? min : j;
                max = (max > j) ? max : j;
            }
        }
    }
    if (min > max) {
        return 0;
    }
    return (max - min + 1);
}

unsigned int Tetrad::height() const {
    unsigned int min = 4;
    unsigned int max = 0;

    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            if (shape_.layout_[i][j] == 1) {
                min = (min < i) ? min : i;
                max = (max > i) ? max : i;
            }
        }
    }
    if (min > max) {
        return 0;
    }
    return (max - min + 1);
}
