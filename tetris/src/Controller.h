/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/engine/Engine.h>
#include <api/event/Event.h>
#include <api/log/Logger.h>
#include <api/ui/Engine.h>
#include <api/ui/shell/GameMenu.h>

#include <string>

#include "Tetrad.h"

class TetrisScene;
class TetrisRenderer;

class Controller final : public v3d::engine::Engine {
 public:
    explicit Controller(const std::string& path);

    bool initialize();
    /**
     * @return bool
     **/
    bool simulate(float step) override;

    /**
     **/
    bool render();

    /**
     * @return bool
     **/
    bool shutdown();

    void handleEvent(const v3d::event::Event& event);

 private:
    /**
     * Move the falling tetrad sideways, if the board has room for it there.
     **/
    void slide(int columns);

    /**
     * Turn the falling tetrad, if the board has room for the turned shape.
     *
     * A turn that would overlap a wall is tried again one and two cells to either side
     * before it is given up on, so that a shape against the edge of the well can still be
     * turned rather than being stuck flat against it.
     **/
    void rotate(Tetrad::RotationDirection direction);

    boost::shared_ptr<TetrisScene> scene_;
    boost::shared_ptr<TetrisRenderer> renderer_;
    boost::shared_ptr<v3d::ui::Engine> vgui_;
    boost::shared_ptr<v3d::ui::shell::GameMenu> menu_;
};
