/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Controller.h"

#include <string>

#include "TetrisScene.h"
#include "Renderer.h"

#include "../../v3dlibs/hookah/Hookah.h"
#include "../../v3dlibs/command/BindLoader.h"

#include <boost/bind.hpp>
#include <boost/property_tree/xml_parser.hpp>

Controller::Controller() {
    // create new app window and set caption
    window_ = Hookah::Create3DWindow(800, 600);

    // create input devices
    keyboard_ = boost::dynamic_pointer_cast<v3d::input::KeyboardDevice, v3d::input::InputDevice>(Hookah::CreateInputDevice("keyboard"));

    // register directory as an observer of input device events
    listenerAdapter_.reset(new v3d::input::InputEventAdapter(keyboard_, mouse_));
    listenerAdapter_->connect(&directory_);

    // add device to window
    window_->addInputDevice("keyboard", keyboard_);

    // set window caption
    window_->caption("Tetris!");

    // load config file into a property tree
    boost::property_tree::ptree ptree;
    boost::property_tree::read_xml("config.xml", ptree);

    // setup scene
    scene_.reset(new TetrisScene());

    renderer_.reset(new TetrisRenderer(scene_));

    // register game commands
    directory_.add("movePieceLeft", "tetris", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_.add("movePieceRight", "tetris", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_.add("rotatePieceCW", "tetris", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_.add("rotatePieceCCW", "tetris", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_.add("dropPiece", "tetris", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_.add("debugMode", "tetris", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));

    // register event listeners
    window_->addDrawListener(boost::bind(&TetrisRenderer::draw, boost::ref(renderer_), _1));
    window_->addResizeListener(boost::bind(&TetrisRenderer::resize, boost::ref(renderer_), _1, _2));
    window_->addTickListener(boost::bind(&TetrisScene::tick, boost::ref(scene_), _1));

    // set the scene size according to the window canvas
    renderer_->resize(window_->width(), window_->height());

    // load key binds from the property tree
    v3d::utility::load_binds(ptree, &directory_);
}

bool Controller::run() {
    return window_->run(Hookah::Window::EVENT_HANDLING_NONBLOCKING);
}

bool Controller::exec(const v3d::command::CommandInfo & command, const std::string & param) {
    if (command.name() == "movePieceLeft") {
        // get the piece to move
        Tetrad & piece = scene_->board()->currentTetrad();

        unsigned int offset = piece.offset(Tetrad::OFFSET_X);

        // do collision detection
        Tetrad::PositionType pos = piece.position();

        // at edge of board already
        if ((pos.first + offset) == 0) {
            return false;
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
            return false;
        }

        piece.move(-1, 0);
        return false;
    } else if (command.name() == "movePieceRight") {
        Tetrad & piece = scene_->board()->currentTetrad();
        Tetrad::PositionType pos = piece.position();

        unsigned int width = piece.width();
        unsigned int offset = piece.offset(Tetrad::OFFSET_X);

        // already at edge of board?
        if (pos.first + width >= scene_->board()->columns()) {
            return false;
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
            return false;
        }

        piece.move(1, 0);
        return false;
    } else if (command.name() == "rotatePieceCW") {
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
        return false;
    } else if (command.name() == "rotatePieceCCW") {
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
        return false;
    } else if (command.name() == "dropPiece") {
        scene_->board()->dropTetrad();
        return false;
    } else if (command.name() == "debugMode") {
        scene_->debug(!scene_->debug());
        scene_->board()->debug(scene_->debug());
        return false;
    }
    return true;
}
