/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/log/Logger.h>

#include <vector>

#include "Piece.h"
#include "Tetrad.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

/**
 * Tetris game board
 */
class GameBoard {
 public:
        explicit GameBoard(const boost::shared_ptr<v3d::log::Logger>& logger);

        /**
         * Read the tetrad shapes the board spawns from.
         *
         * This is separate from construction because it can fail. A board with no shapes has
         * nothing to spawn, and the caller has to report that.
         *
         * @param assetManager where pieces/shapes.txt is resolved against
         * @return false when no shape could be read
         */
        bool load(const boost::shared_ptr<v3d::asset::Manager>& assetManager);

        /**
         * Install a set of shapes directly. The load from a file ends by calling this.
         *
         * A test can use this to give the board shapes without a file.
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
         * @return the points scored this game, a hundred for each row cleared
         */
        unsigned int score() const;

        /**
         * @return whether a tetrad spawned onto blocks that were already there
         */
        bool over() const;

        /**
         * Toggle the falling speed of the current tetrad.
         * @param repeated whether this is a held key repeating rather than a new press. A
         *        repeat leaves the speed as it is, so holding the key does not flick it
         * @return true if the tetrad is falling quickly or false if slowly.
         */
        bool dropTetrad(bool repeated = false);

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
        void piece(unsigned int col, unsigned int row, const Piece & placed);

        Tetrad currentTetrad() const;
        Tetrad nextTetrad() const;

        /**
         * Whether a tetrad's shape would fit at a given position: inside the walls and the
         * floor, and over no block already on the board. An uninitialized tetrad never fits.
         *
         * The spawn, the fall, the sideways moves and the rotation are all checked through
         * this, so they share one definition of a legal position.
         *
         * @param tetrad the shape to test, whose own position is ignored
         * @param cell the column and row of the top left corner of its 4x4 layout
         */
        bool fits(const Tetrad & tetrad, const glm::ivec2 & cell) const;

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
