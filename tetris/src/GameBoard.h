/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Piece.h"
#include "Tetrad.h"

#include <vector>

/**
 * Tetris game board
 */
class GameBoard {
 public:
        GameBoard();

        void update(unsigned int delta);

        /**
         * Get the currently falling tetrad.
         * @return a reference to the currently active tetrad
         */
        Tetrad & currentTetrad();

        /**
         * Get a random tetrad with a random orientation.
         * @return a random tetrad
         */
        Tetrad randomTetrad();

        /**
         * Get the number of columns in the board. Default is 10.
         * @return the number of columns
         */
        unsigned int columns() const;
        /**
         * Get the number of rows in the board. Default is 20.
         * @return the number of rows
         */
        unsigned int rows() const;

        void debug(bool dbg);
        bool debug() const;

        /**
         * toggle the falling speed of the current tetrad.
         * @return true if the tetrad is falling quickly or false if slowly.
         */
        bool dropTetrad();

        /**
         * Get a piece on the board.
         * @param col column on the board (x axis)
         * @param row row on the board (y axis)
         */
        Piece piece(unsigned int col, unsigned int row) const;

        Tetrad currentTetrad() const;
        Tetrad nextTetrad() const;

 protected:
        void loadShapeInfo();
        void spawnTetrad();
        void checkCompletedRows();

 private:
        std::vector< std::vector<Piece> > pieces_;  // [rows][cols]
        unsigned int rows_;
        unsigned int cols_;

        unsigned int fallRate_;
        unsigned int fastFallMultiplier_;
        bool fastFall_;

        Tetrad currentTetrad_;
        Tetrad nextTetrad_;
        std::vector<Tetrad::ShapeInfo> shapes_;
        bool debug_;
};
