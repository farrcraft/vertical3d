/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

/**
 * An individual block on the tetris board. Once a tetrad has finished falling 
 * and is 'locked' into place it is broken down into its constituent pieces. 
 * These individual blocks are recorded on the game board
 */
class Piece {
 public:
    typedef enum {
        COLOR_EMPTY,
        COLOR_RED,
        COLOR_CYAN,
        COLOR_BLUE,
        COLOR_GREEN,
        COLOR_ORANGE,
        COLOR_PURPLE,
        COLOR_YELLOW
    } ColorType;

    explicit Piece(ColorType type = COLOR_EMPTY);
    explicit Piece(std::string color_name);

    /**
     * Get the enumerated color value for this piece.
     * @return the enumerated color value.
     */
    ColorType color() const;
    /**
     * Get the color name for this piece.
     * @return a string representing the color name.
     */
    std::string str() const;

 private:
      ColorType color_;
};

