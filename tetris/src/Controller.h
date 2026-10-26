/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/engine/Engine.h>
#include <api/event/Event.h>
#include <api/log/Logger.h>
#include <api/ui/Engine.h>
#include <api/shell/Shell.h>

#include <string>

#include "Tetrad.h"

class TetrisScene;
class TetrisRenderer;

class Controller final : public v3d::engine::Engine {
 public:
    explicit Controller(const std::string& path);

    /**
     * @return bool
     **/
    bool simulate(float step) override;

    /**
     **/
    bool render() override;

 protected:
    bool start() override;

    /**
     * @return bool
     **/
    bool release() override;

 private:
    /**
     * Move the falling tetrad sideways, if the board has room for it there.
     **/
    void slide(int columns);

    /**
     * Turn the falling tetrad, if the board has room for the turned shape.
     *
     * A turned shape that does not fit is tried again one and then two cells to either side.
     * A shape against the side of the well can then still turn.
     **/
    void rotate(Tetrad::RotationDirection direction);

    boost::shared_ptr<TetrisScene> scene_;
    boost::shared_ptr<TetrisRenderer> renderer_;
    // the ui, the menu, the screen and the statistics overlay
    v3d::shell::Shell shell_;
};
