/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <string>
#include <utility>  // for std::pair

/**
 * A shape of blocks in a 4x4 grid, with a position on the board and an orientation. The
 * falling piece and the next piece are both tetrads.
 */
class Tetrad {
 public:
        class ShapeInfo {
         public:
            uint8_t layout_[4][4];  // internal bitmap
            std::string color_;  // texture name
        };

        typedef enum : std::uint8_t {
            OFFSET_X = 0,
            OFFSET_Y = 1,
            OFFSET_COL = 1,
            OFFSET_ROW = 0
        } OffsetAxis;

        typedef enum : std::uint8_t {
            CLOCKWISE,
            COUNTERCLOCKWISE
        } RotationDirection;

        typedef std::pair<unsigned int, unsigned int> PositionType;

        Tetrad();
        explicit Tetrad(const ShapeInfo & shape);

        /**
         * Set the position of the tetrad.
         * @param newPosition the new position
         */
        void position(PositionType newPosition);
        /**
         * Move the tetrad by the specified amount.
         * @param dx amount to move the tetrad in the x axis (number of columns)
         * @param dy amount to move the tetrad in the y axis (number of rows)
         */
        void move(int dx, int dy);
        /**
         * Get the current tetrad position
         * @return the current position
         */
        PositionType position() const;
        unsigned int orientation() const;
        void orientation(unsigned int newOrientation);
        void rotate(RotationDirection dir);
        unsigned int width() const;
        unsigned int height() const;
        bool initialized() const;
        const ShapeInfo & shape() const;

        /**
         * Get the offset of the shape from the top left of the 4x4 grid
         * @param dir the offset axis to be returned
         * @return the offset in the given axis
         */
        unsigned int offset(OffsetAxis dir) const;

        Tetrad & operator=(const Tetrad & t);

        /**
         * Move a shape's filled cells into the top left of its 4x4 grid.
         *
         * Every shape is kept normalised, and rotate() normalises its result. A shape that
         * started out unnormalised would otherwise stop short of the left wall until its first
         * turn.
         *
         * @param shape the layout to normalise in place
         */
        static void normalize(ShapeInfo * shape);

 private:
        unsigned int orientation_;  // 0 = 0, 1 = 90, 2 = 180, 3 = 270 degree rotation
        PositionType position_;  // where on the board
        // unsigned int _width;
        // unsigned int _height;
        ShapeInfo shape_;
        bool initialized_;
};
