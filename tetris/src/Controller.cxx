/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Controller.h"

#include <string>

#include "TetrisScene.h"
#include "Renderer.h"
#include "../../api/engine/Feature.h"

#include <boost/make_shared.hpp>

Controller::Controller(const std::string& path) : v3d::engine::Engine(path) {
    logger_ = boost::make_shared<v3d::log::Logger>();
}

bool Controller::initialize() {
    if (!Engine::initialize(
        static_cast<int>(v3d::engine::Feature::Window3D |
            v3d::engine::Feature::KeyboardInput |
            v3d::engine::Feature::MouseInput |
            v3d::engine::Feature::Config))) {
        return false;
    }

    window_->caption("Tetris!");

    // setup scene
    scene_ = boost::make_shared<TetrisScene>();

    renderer_ = boost::make_shared<TetrisRenderer>(scene_, logger_);

    // register game commands
    dispatcher_->sink<v3d::event::Event>().connect<&Controller::handleEvent>(*this);
    /*
    directory_.add("movePieceLeft", "tetris", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_.add("movePieceRight", "tetris", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_.add("rotatePieceCW", "tetris", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_.add("rotatePieceCCW", "tetris", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_.add("dropPiece", "tetris", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_.add("debugMode", "tetris", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    */

    // register event listeners
    /*
    window_->addDrawListener(boost::bind(&TetrisRenderer::draw, boost::ref(renderer_), _1));
    window_->addResizeListener(boost::bind(&TetrisRenderer::resize, boost::ref(renderer_), _1, _2));
    window_->addTickListener(boost::bind(&TetrisScene::tick, boost::ref(scene_), _1));
    */

    // set the scene size according to the window canvas
    renderer_->resize(window_->width(), window_->height());

    // load key binds from the property tree
    // v3d::utility::load_binds(ptree, &directory_);
}

/**
 **/
bool Controller::tick() {
    if (!v3d::engine::Engine::tick()) {
        return false;
    }
    // scene_->tick();
    return true;
}

bool Controller::render() {
    return true;
}

/**
 **/
bool Controller::shutdown() {
    if (!v3d::engine::Engine::shutdown()) {
        return false;
    }
    return true;
}

void Controller::handleEvent(const v3d::event::Event& event) {
    if (event.name() == "movePieceLeft") {
        // get the piece to move
        Tetrad & piece = scene_->board()->currentTetrad();

        unsigned int offset = piece.offset(Tetrad::OFFSET_X);

        // do collision detection
        Tetrad::PositionType pos = piece.position();

        // at edge of board already
        if ((pos.first + offset) == 0) {
            return;
        }

        // is there a piece to the left that would block this one?
        bool hit = false;
        for (unsigned int k = 0; k < 4; k++) {
            Piece p = scene_->board()->piece(pos.first - 1, pos.second + k);
            if (p.color() != Piece::COLOR_EMPTY) {
                if (piece.shape().layout_[0][k] == 1) {
                    hit = true;
                }
            }
        }
        if (hit) {
            return;
        }

        piece.move(-1, 0);
        return;
    } else if (event.name() == "movePieceRight") {
        Tetrad & piece = scene_->board()->currentTetrad();
        Tetrad::PositionType pos = piece.position();

        unsigned int width = piece.width();
        unsigned int offset = piece.offset(Tetrad::OFFSET_X);

        // already at edge of board?
        if (pos.first + width >= scene_->board()->columns()) {
            return;
        }

        // is there a piece to the right that would block this one?
        bool hit = false;
        /*
            look at each block to the right of this tetrad

        */
        for (unsigned int k = 0; k < 4; k++) {
            Piece p = scene_->board()->piece(pos.first + 2, pos.second + k);
            if (p.color() != Piece::COLOR_EMPTY) {
                if (piece.shape().layout_[3][k] == 1) {
                    hit = true;
                }
            }
        }
        if (hit) {
            return;
        }

        piece.move(1, 0);
        return;
    } else if (event.name() == "rotatePieceCW") {
        Tetrad & piece = scene_->board()->currentTetrad();
        unsigned int orient = piece.orientation();
        if (orient > 0) {
            orient--;
        } else {
            orient = 3;
        }
        piece.orientation(orient);
        piece.rotate(Tetrad::CLOCKWISE);
        Tetrad::PositionType pos = piece.position();
        if (pos.first + piece.width() > scene_->board()->columns()) {
            piece.move((scene_->board()->columns() - (pos.first + piece.width())), 0);
        }
        return;
    } else if (event.name() == "rotatePieceCCW") {
        Tetrad & piece = scene_->board()->currentTetrad();
        unsigned int orient = piece.orientation();
        if (orient < 3) {
            orient++;
        } else {
            orient = 0;
        }
        piece.orientation(orient);
        piece.rotate(Tetrad::COUNTERCLOCKWISE);
        Tetrad::PositionType pos = piece.position();
        if (pos.first + piece.width() > scene_->board()->columns()) {
            piece.move((scene_->board()->columns() - (pos.first + piece.width())), 0);
        }
        return;
    } else if (event.name() == "dropPiece") {
        scene_->board()->dropTetrad();
        return;
    } else if (event.name() == "debugMode") {
        scene_->debug(!scene_->debug());
        scene_->board()->debug(scene_->debug());
        return;
    }
    return;
}
