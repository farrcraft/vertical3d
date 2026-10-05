/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Controller.h"

#include <api/engine/Feature.h>

#include <string>

#include "TetrisScene.h"
#include "Renderer.h"

#include <boost/make_shared.hpp>

Controller::Controller(const std::string& path) : v3d::engine::Engine(path) {
    logger_ = boost::make_shared<v3d::log::Logger>();
}

bool Controller::start() {
    window_->caption("Tetris!");

    vgui_ = boost::make_shared<v3d::ui::Engine>(eventEngine_, dispatcher_, logger_);
    menu_ = boost::make_shared<v3d::ui::shell::GameMenu>(vgui_, [this](bool suspended) {
        scene_->pause(suspended);
    });
    const boost::json::object* ui = document(v3d::config::Type::Ui);
    if (ui && !vgui_->load(*ui)) {
        return false;
    }

    scene_ = boost::make_shared<TetrisScene>(logger_);
    if (!scene_->load(assetManager_)) {
        return false;
    }

    boost::shared_ptr<v3d::render::realtime::Window> win = window();
    renderer_ = boost::make_shared<TetrisRenderer>(win, logger_, assetManager_);
    renderer_->scene(scene_);
    renderer_->ui(vgui_);

    // register game commands
    events_ = dispatcher_->sink<v3d::event::Event>().connect<&Controller::handleEvent>(*this);

    // set the scene size according to the window canvas
    renderer_->resize(window_->width(), window_->height());

    scene_->reset();

    return true;
}

/**
 **/
bool Controller::simulate(float step) {
    if (!v3d::engine::Engine::simulate(step)) {
        return false;
    }
    scene_->tick(step);
    return true;
}

bool Controller::render() {
    const v3d::engine::Statistics& measured = statistics();
    renderer_->draw({ measured.mean(), measured.last(), measured.steps() });
    return true;
}

/**
 **/
bool Controller::release() {
    if (renderer_) {
        // the device has to be idle before the window it presents to is destroyed
        renderer_->shutdown();
    }
    return true;
}

/**
 **/
void Controller::slide(int columns) {
    GameBoard* board = scene_->board();
    Tetrad& piece = board->currentTetrad();
    const Tetrad::PositionType position = piece.position();

    if (board->fits(piece, static_cast<int>(position.first) + columns, static_cast<int>(position.second))) {
        piece.move(columns, 0);
    }
}

/**
 **/
void Controller::rotate(Tetrad::RotationDirection direction) {
    GameBoard* board = scene_->board();
    Tetrad& piece = board->currentTetrad();
    if (!piece.initialized()) {
        return;
    }

    Tetrad turned = piece;
    turned.rotate(direction);

    const Tetrad::PositionType position = piece.position();
    const int row = static_cast<int>(position.second);
    const int column = static_cast<int>(position.first);
    const int kicks[] = { 0, -1, 1, -2, 2 };
    for (int kick : kicks) {
        if (board->fits(turned, column + kick, row)) {
            turned.position(Tetrad::PositionType(static_cast<unsigned int>(column + kick), position.second));
            piece = turned;
            return;
        }
    }
}

void Controller::handleEvent(const v3d::event::Event& event) {
    if (event.context()->name() == "tetris") {
        if (event.name() == "toggleStatistics") {
            renderer_->statistics()->toggle();
            return;
        }
        if (event.name() == "debugMode") {
            scene_->debug(!scene_->debug());
            scene_->board()->debug(scene_->debug());
            return;
        }
        // the board only takes play commands while it is running
        if (scene_->paused() || scene_->board()->over()) {
            return;
        }
        if (event.name() == "movePieceLeft") {
            slide(-1);
        } else if (event.name() == "movePieceRight") {
            slide(1);
        } else if (event.name() == "rotatePieceCW") {
            rotate(Tetrad::CLOCKWISE);
        } else if (event.name() == "rotatePieceCCW") {
            rotate(Tetrad::COUNTERCLOCKWISE);
        } else if (event.name() == "dropPiece") {
            scene_->board()->dropTetrad();
        }
        return;
    }

    if (event.context()->name() == "ui") {
        if (event.name() == "newGame") {
            scene_->reset();
            menu_->toggle();
        }
    }
}
