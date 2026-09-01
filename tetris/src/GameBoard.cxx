/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "GameBoard.h"

#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>

#include "../../api/asset/Text.h"
#include "../../api/asset/Type.h"

namespace {

    /**
     * Points for clearing one row.
     **/
    const unsigned int rowScore = 100;

};  // namespace

GameBoard::GameBoard(const boost::shared_ptr<v3d::log::Logger>& logger) :
                        rows_(20), cols_(10), fallRate_(800),
                        fastFallMultiplier_(8), fastFall_(false), nextMove_(800),
                        debug_(false), score_(0), over_(false), logger_(logger) {
    reset();

    // random seed
    srand((unsigned)time(0));  // NOLINT
}

bool GameBoard::load(const boost::shared_ptr<v3d::asset::Manager>& assetManager) {
    boost::shared_ptr<v3d::asset::Text> file;
    try {
        file = boost::dynamic_pointer_cast<v3d::asset::Text>(assetManager->load("pieces/shapes.txt", v3d::asset::Type::Text));
    } catch (const std::exception& error) {
        logger_->get()->error("unable to read the tetrad shapes - {}", error.what());
        return false;
    }
    if (!file) {
        logger_->get()->error("unable to read the tetrad shapes");
        return false;
    }

    // four rows of a 4x4 bitmap, then the name of the texture that shape is drawn with
    std::istringstream stream(file->content());
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(stream, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
            line.pop_back();
        }
        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    std::vector<Tetrad::ShapeInfo> parsed;
    for (size_t i = 0; i + 4 < lines.size(); i += 5) {
        Tetrad::ShapeInfo info;
        bool valid = true;
        for (unsigned int row = 0; row < 4; row++) {
            if (lines[i + row].size() < 4) {
                valid = false;
                break;
            }
            for (unsigned int column = 0; column < 4; column++) {
                info.layout_[row][column] = (lines[i + row][column] == '1') ? 1 : 0;
            }
        }
        if (!valid) {
            continue;
        }
        info.color_ = lines[i + 4];
        parsed.push_back(info);
    }

    if (!load(parsed)) {
        logger_->get()->error("no tetrad shapes were read from pieces/shapes.txt");
        return false;
    }
    logger_->get()->info("loaded {} tetrad shapes", shapes_.size());
    return true;
}

bool GameBoard::load(const std::vector<Tetrad::ShapeInfo>& shapes) {
    if (shapes.empty()) {
        return false;
    }
    shapes_ = shapes;
    for (Tetrad::ShapeInfo & shape : shapes_) {
        Tetrad::normalize(&shape);
    }
    return true;
}

void GameBoard::reset() {
    pieces_.clear();
    for (unsigned int i = 0; i < rows_; i++) {
        pieces_.push_back(std::vector<Piece>(cols_));
    }
    currentTetrad_ = Tetrad();
    nextTetrad_ = Tetrad();
    fastFall_ = false;
    nextMove_ = fallRate_;
    score_ = 0;
    over_ = false;
}

bool GameBoard::dropTetrad() {
    fastFall_ = !fastFall_;
    return fastFall_;
}

bool GameBoard::debug() const {
    return debug_;
}

void GameBoard::debug(bool dbg) {
    debug_ = dbg;
}

unsigned int GameBoard::score() const {
    return score_;
}

bool GameBoard::over() const {
    return over_;
}

unsigned int GameBoard::columns() const {
    return cols_;
}

unsigned int GameBoard::rows() const {
    return rows_;
}

Piece GameBoard::piece(unsigned int col, unsigned int row) const {
    if (row >= pieces_.size() || col >= pieces_[row].size()) {
        return Piece();
    }
    return pieces_[row][col];
}

void GameBoard::piece(unsigned int col, unsigned int row, const Piece & p) {
    if (row >= pieces_.size() || col >= pieces_[row].size()) {
        return;
    }
    pieces_[row][col] = p;
}

Tetrad GameBoard::currentTetrad() const {
    return currentTetrad_;
}

Tetrad GameBoard::nextTetrad() const {
    return nextTetrad_;
}

Tetrad & GameBoard::currentTetrad() {
    return currentTetrad_;
}

bool GameBoard::fits(const Tetrad & tetrad, int column, int row) const {
    if (!tetrad.initialized()) {
        return false;
    }
    const Tetrad::ShapeInfo & shape = tetrad.shape();
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (shape.layout_[i][j] == 0) {
                continue;
            }
            const int boardColumn = column + j;
            const int boardRow = row + i;
            if (boardColumn < 0 || boardColumn >= static_cast<int>(cols_) ||
                boardRow < 0 || boardRow >= static_cast<int>(rows_)) {
                return false;
            }
            if (pieces_[boardRow][boardColumn].color() != Piece::COLOR_EMPTY) {
                return false;
            }
        }
    }
    return true;
}

Tetrad GameBoard::randomTetrad() {
    // pick a shape
    size_t shapeCount = shapes_.size();
    size_t num = rand() % shapeCount;  // NOLINT

    Tetrad piece(shapes_[num]);
    // and an initial rotation
    unsigned int rot = rand() % 4;  // NOLINT
    for (unsigned int i = 0; i < rot; i++) {
        piece.rotate(Tetrad::CLOCKWISE);
    }

    return piece;
}

void GameBoard::spawnTetrad() {
    if (shapes_.empty()) {
        return;
    }
    if (!nextTetrad_.initialized()) {
        nextTetrad_ = randomTetrad();
    }

    // reset default fall speed flag
    fastFall_ = false;
    nextMove_ = fallRate_;

    currentTetrad_ = nextTetrad_;
    nextTetrad_ = randomTetrad();

    // place it centered at the top of the board. A shape is normalised into the top left of
    // its 4x4 grid, so the grid's left column is also the shape's
    const Tetrad::PositionType position((cols_ - 4) / 2, 0);
    currentTetrad_.position(position);

    // nowhere left to put it: the stack has reached the top
    if (!fits(currentTetrad_, position.first, position.second)) {
        over_ = true;
    }
}

void GameBoard::lockTetrad() {
    const Tetrad::ShapeInfo & shape = currentTetrad_.shape();
    const Tetrad::PositionType position = currentTetrad_.position();

    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            if (shape.layout_[i][j] == 0) {
                continue;
            }
            const unsigned int row = position.second + i;
            const unsigned int column = position.first + j;
            // fits() has already said the tetrad is inside the board, but a lock is the one
            // place a stray write would corrupt the heap rather than draw something odd
            if (row < rows_ && column < cols_) {
                pieces_[row][column] = Piece(shape.color_);
            }
        }
    }
}

void GameBoard::update(unsigned int delta) {
    if (over_ || shapes_.empty()) {
        return;
    }

    // no current tetrad so spawn a new one
    if (!currentTetrad_.initialized()) {
        spawnTetrad();
        return;
    }

    nextMove_ -= static_cast<int>(delta);
    if (nextMove_ > 0) {
        return;
    }
    nextMove_ = fastFall_ ? (fallRate_ / fastFallMultiplier_) : fallRate_;

    const Tetrad::PositionType position = currentTetrad_.position();
    if (fits(currentTetrad_, position.first, position.second + 1)) {
        currentTetrad_.move(0, 1);
        return;
    }

    // it has landed, either on the floor or on what is already stacked up
    lockTetrad();
    score_ += checkCompletedRows() * rowScore;
    spawnTetrad();
}

unsigned int GameBoard::checkCompletedRows() {
    unsigned int cleared = 0;

    // from the bottom up, and a cleared row is looked at again rather than stepped past,
    // because what was above it has just been shifted into it
    unsigned int row = rows_;
    while (row > 0) {
        unsigned int filled = 0;
        for (unsigned int j = 0; j < cols_; j++) {
            if (pieces_[row - 1][j].color() != Piece::COLOR_EMPTY) {
                filled++;
            }
        }
        if (filled != cols_) {
            row--;
            continue;
        }

        // move everything above down one row and empty the top one
        for (unsigned int m = row - 1; m > 0; m--) {
            pieces_[m] = pieces_[m - 1];
        }
        pieces_[0] = std::vector<Piece>(cols_);
        cleared++;
    }

    return cleared;
}
