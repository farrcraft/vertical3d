/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <tetris/src/GameBoard.h>

#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * A one cell shape, so that a test can put exactly one block where it wants it. A
 * rotation of a single cell normalises back to the same layout, which the board's random
 * initial rotation relies on here.
 **/
Tetrad::ShapeInfo dot() {
    Tetrad::ShapeInfo shape;
    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            shape.layout_[i][j] = 0;
        }
    }
    shape.layout_[0][0] = 1;
    shape.color_ = "red";
    return shape;
}

/**
 * A 2x1 horizontal bar, for the cases that need a shape wider than one cell.
 **/
Tetrad::ShapeInfo bar() {
    Tetrad::ShapeInfo shape = dot();
    shape.layout_[0][1] = 1;
    shape.color_ = "blue";
    return shape;
}

boost::shared_ptr<GameBoard> board() {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    boost::shared_ptr<GameBoard> game = boost::make_shared<GameBoard>(logger);
    std::vector<Tetrad::ShapeInfo> shapes;
    shapes.push_back(dot());
    game->load(shapes);
    return game;
}

};  // namespace

BOOST_AUTO_TEST_CASE(gameboard_fits_test) {
    boost::shared_ptr<GameBoard> game = board();
    Tetrad piece(bar());

    BOOST_CHECK_EQUAL(game->fits(piece, 0, 0), true);
    // hard against either wall is still inside it
    BOOST_CHECK_EQUAL(game->fits(piece, static_cast<int>(game->columns()) - 2, 0), true);
    // and one cell further is not
    BOOST_CHECK_EQUAL(game->fits(piece, static_cast<int>(game->columns()) - 1, 0), false);
    BOOST_CHECK_EQUAL(game->fits(piece, -1, 0), false);
    // the floor
    BOOST_CHECK_EQUAL(game->fits(piece, 0, static_cast<int>(game->rows()) - 1), true);
    BOOST_CHECK_EQUAL(game->fits(piece, 0, static_cast<int>(game->rows())), false);

    // a block already on the board, which the two cell bar overlaps from either side
    game->piece(4, 5, Piece(Piece::COLOR_GREEN));
    BOOST_CHECK_EQUAL(game->fits(piece, 2, 5), true);
    BOOST_CHECK_EQUAL(game->fits(piece, 3, 5), false);
    BOOST_CHECK_EQUAL(game->fits(piece, 4, 5), false);
    // and the row below it is still clear
    BOOST_CHECK_EQUAL(game->fits(piece, 3, 6), true);

    // an uninitialized tetrad is nowhere
    BOOST_CHECK_EQUAL(game->fits(Tetrad(), 0, 0), false);
}

BOOST_AUTO_TEST_CASE(gameboard_row_clear_test) {
    boost::shared_ptr<GameBoard> game = board();
    const unsigned int bottom = game->rows() - 1;
    const unsigned int last = game->columns() - 1;

    // every cell of the bottom row but the last, and one block floating above so the shift
    // down after the clear has something to move. It goes in a different column from the
    // gap, or it would catch the falling piece before it reached the floor
    for (unsigned int column = 0; column < last; column++) {
        game->piece(column, bottom, Piece(Piece::COLOR_GREEN));
    }
    game->piece(0, bottom - 2, Piece(Piece::COLOR_PURPLE));

    // spawn, then steer the one cell piece over the gap and let it fall into it
    game->update(0.0f);
    game->currentTetrad().position(Tetrad::PositionType(last, 0));
    for (unsigned int i = 0; i < 64 && game->score() == 0; i++) {
        game->update(1.0f);
    }

    BOOST_CHECK_EQUAL(game->score(), 100u);
    // the completed row is gone rather than merely emptied, so what was two rows above the
    // gap is now one row above the floor
    BOOST_CHECK_EQUAL(game->piece(0, bottom).color(), Piece::COLOR_EMPTY);
    BOOST_CHECK_EQUAL(game->piece(0, bottom - 1).color(), Piece::COLOR_PURPLE);
    BOOST_CHECK_EQUAL(game->over(), false);
}

BOOST_AUTO_TEST_CASE(gameboard_incomplete_row_test) {
    boost::shared_ptr<GameBoard> game = board();
    const unsigned int bottom = game->rows() - 1;

    // one short of a full row: nothing clears, and the block that lands stays put
    for (unsigned int column = 0; column + 2 < game->columns(); column++) {
        game->piece(column, bottom, Piece(Piece::COLOR_GREEN));
    }

    game->update(0.0f);
    game->currentTetrad().position(Tetrad::PositionType(game->columns() - 1, 0));
    for (unsigned int i = 0; i < 64; i++) {
        game->update(1.0f);
    }

    BOOST_CHECK_EQUAL(game->score(), 0u);
    BOOST_CHECK_EQUAL(game->piece(0, bottom).color(), Piece::COLOR_GREEN);
}

BOOST_AUTO_TEST_CASE(gameboard_game_over_test) {
    boost::shared_ptr<GameBoard> game = board();

    // the whole well full, so the next spawn has nowhere to go
    for (unsigned int row = 0; row < game->rows(); row++) {
        for (unsigned int column = 0; column < game->columns(); column++) {
            game->piece(column, row, Piece(Piece::COLOR_GREEN));
        }
    }

    game->update(0.0f);
    BOOST_CHECK_EQUAL(game->over(), true);

    // and a game that is over does not keep running
    const unsigned int score = game->score();
    game->update(10.0f);
    BOOST_CHECK_EQUAL(game->score(), score);
}

BOOST_AUTO_TEST_CASE(gameboard_load_test) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    GameBoard game(logger);

    BOOST_CHECK_EQUAL(game.load(std::vector<Tetrad::ShapeInfo>()), false);

    // a shape sitting away from the corner is normalised on the way in, so that it reaches
    // the left wall before its first rotation rather than after it
    Tetrad::ShapeInfo offset = dot();
    offset.layout_[0][0] = 0;
    offset.layout_[2][2] = 1;
    std::vector<Tetrad::ShapeInfo> shapes;
    shapes.push_back(offset);
    BOOST_REQUIRE_EQUAL(game.load(shapes), true);

    game.update(0.0f);
    BOOST_CHECK_EQUAL(game.currentTetrad().shape().layout_[0][0], 1);
}

/**
 * The fall rate is a duration rather than a number of steps, so a second of simulated time
 * drops a tetrad the same distance however that second was divided up. This is what the
 * board was not doing while update() counted whole milliseconds: a step shorter than one
 * rounded to zero and the piece never fell at all.
 **/
BOOST_AUTO_TEST_CASE(gameboard_fall_rate_is_a_duration_test) {
    boost::shared_ptr<GameBoard> coarse = board();
    boost::shared_ptr<GameBoard> fine = board();

    coarse->update(0.0f);
    fine->update(0.0f);
    coarse->currentTetrad().position(Tetrad::PositionType(0, 0));
    fine->currentTetrad().position(Tetrad::PositionType(0, 0));

    // four seconds, at 60 Hz and at 240 Hz
    for (unsigned int i = 0; i < 240; i++) {
        coarse->update(1.0f / 60.0f);
    }
    for (unsigned int i = 0; i < 960; i++) {
        fine->update(1.0f / 240.0f);
    }

    BOOST_CHECK_EQUAL(coarse->currentTetrad().position().second,
                      fine->currentTetrad().position().second);
    // and it actually fell, so the comparison is not two pieces sitting at the top
    BOOST_CHECK_GT(coarse->currentTetrad().position().second, 0);
}
