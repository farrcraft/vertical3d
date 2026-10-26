/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Controller.h"

#include <api/config/Type.h>
#include <api/engine/Engine.h>
#include <api/engine/Statistics.h>
#include <api/event/Event.h>
#include <api/ui/Engine.h>
#include <api/ui/shell/GameMenu.h>

#include <string>

#include "Tetrad.h"
#include "TetrisScene.h"
#include "Renderer.h"

#include <boost/make_shared.hpp>

Controller::Controller(const std::string& path) : v3d::engine::Engine(path) {
    v3d::ui::shell::GameMenu::declare(commands(), menu_);
    commands().addPress("tetris::toggleStatistics", [this]() {
        renderer_->statistics()->toggle();
    });
    commands().addPress("tetris::debugMode", [this]() {
        scene_->debug(!scene_->debug());
        scene_->board()->debug(scene_->debug());
    });

    // a play command acts on a held key's repeats too, so a piece keeps sliding while its key
    // is down. Play commands do nothing while the game is paused or over
    auto play = [this](const std::string& name, const v3d::event::Commands::Handler& action) {
        commands().add(name, [this, action](const v3d::event::Event& event) {
            if (!scene_->paused() && !scene_->board()->over()) {
                action(event);
            }
        });
    };
    play("tetris::movePieceLeft", [this](const v3d::event::Event&) { slide(-1); });
    play("tetris::movePieceRight", [this](const v3d::event::Event&) { slide(1); });
    play("tetris::rotatePieceCW", [this](const v3d::event::Event&) { rotate(Tetrad::CLOCKWISE); });
    play("tetris::rotatePieceCCW", [this](const v3d::event::Event&) { rotate(Tetrad::COUNTERCLOCKWISE); });
    play("tetris::dropPiece", [this](const v3d::event::Event& event) {
        scene_->board()->dropTetrad(event.repeat());
    });

    // what the menu's items send
    commands().addPress("ui::newGame", [this]() {
        scene_->reset();
        menu_->toggle();
    });
}

bool Controller::start() {
    window()->caption("Tetris!");

    vgui_ = boost::make_shared<v3d::ui::Engine>(events(), dispatcher(), logger());
    menu_ = boost::make_shared<v3d::ui::shell::GameMenu>(vgui_, [this](bool suspended) {
        scene_->pause(suspended);
    });
    const boost::json::object* ui = document(v3d::config::Type::Ui);
    if (ui != nullptr && !vgui_->load(*ui)) {
        return false;
    }

    scene_ = boost::make_shared<TetrisScene>(logger());
    if (!scene_->load(assets())) {
        return false;
    }

    const boost::shared_ptr<v3d::window::Window> win = window();
    renderer_ = boost::make_shared<TetrisRenderer>(win, logger(), assets());
    renderer_->scene(scene_);
    renderer_->ui(vgui_);

    // TetrisScene::resize() ignores the size. TetrisRenderer::layout() places the well from the
    // canvas size on every frame
    renderer_->resize(window()->width(), window()->height());

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

    if (board->fits(piece, glm::ivec2(static_cast<int>(position.first) + columns, static_cast<int>(position.second)))) {
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
    for (const int kick : kicks) {
        if (board->fits(turned, glm::ivec2(column + kick, row))) {
            turned.position(Tetrad::PositionType(static_cast<unsigned int>(column + kick), position.second));
            piece = turned;
            return;
        }
    }
}
