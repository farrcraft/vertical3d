/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "Piece.h"
#include "Tetrad.h"

#include "../../api/asset/Manager.h"
#include "../../api/log/Logger.h"

#include <boost/shared_ptr.hpp>

/**
 * Tetris game board
 */
class GameBoard {
 public:
        explicit GameBoard(const boost::shared_ptr<v3d::log::Logger>& logger);

        /**
         * Read the tetrad shapes the board spawns from.
         *
         * Separate from construction because it can fail: a board with no shapes has
         * nothing to spawn, and the caller has to be able to say so.
         *
         * @param assetManager where pieces/shapes.txt is resolved against
         * @return false when no shape could be read
         */
        bool load(const boost::shared_ptr<v3d::asset::Manager>& assetManager);

        /**
         * Install a shape set directly, which is what reading the file ends in.
         *
         * Separate from the read so that what the board does with a set of shapes can be
         * exercised without a file behind it.
         *
         * @param shapes the shapes to spawn from, normalised on the way in
         * @return false when the set is empty
         */
        bool load(const std::vector<Tetrad::ShapeInfo>& shapes);

        /**
         * Clear the board, the score and the falling tetrad, ready for a new game.
         */
        void reset();

        /**
         * Advance the falling tetrad by one simulation step.
         * @param step seconds of simulated time
         */
        void update(float step);

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
         * @return how many rows have been cleared, ten points apiece
         */
        unsigned int score() const;

        /**
         * @return whether a tetrad spawned onto blocks that were already there
         */
        bool over() const;

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

        /**
         * Put a piece on the board, ignoring an out of range cell.
         * @param col column on the board (x axis)
         * @param row row on the board (y axis)
         */
        void piece(unsigned int col, unsigned int row, const Piece & p);

        Tetrad currentTetrad() const;
        Tetrad nextTetrad() const;

        /**
         * Whether a tetrad's shape would overlap the walls, the floor or a block already on
         * the board if it were at a given position.
         *
         * Every move the board or the controller makes is checked through this, so that one
         * description of what a legal position is serves the fall, the sideways moves and
         * the rotation.
         *
         * @param tetrad the shape to test, whose own position is ignored
         * @param column the leftmost column of its 4x4 layout
         * @param row the topmost row of its 4x4 layout
         */
        bool fits(const Tetrad & tetrad, int column, int row) const;

 protected:
        void spawnTetrad();

        /**
         * Break the falling tetrad into the pieces it leaves behind on the board.
         */
        void lockTetrad();

        unsigned int checkCompletedRows();

 private:
        std::vector< std::vector<Piece> > pieces_;  // [rows][cols]
        unsigned int rows_;
        unsigned int cols_;

        float fallRate_;  // seconds a tetrad rests on each row
        unsigned int fastFallMultiplier_;
        bool fastFall_;
        float nextMove_;  // seconds remaining until the current tetrad falls again

        Tetrad currentTetrad_;
        Tetrad nextTetrad_;
        std::vector<Tetrad::ShapeInfo> shapes_;
        bool debug_;
        unsigned int score_;
        bool over_;
        boost::shared_ptr<v3d::log::Logger> logger_;
};
