/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/audio/Engine.h>
#include <api/engine/Engine.h>
#include <api/event/Event.h>
#include <api/event/Source.h>
#include <api/shell/Shell.h>

#include <string>

#include "PongScene.h"

class PongRenderer;

/**
 * The core game engine class
 */
class PongEngine final : public v3d::engine::Engine {
 public:
    explicit PongEngine(const std::string & path);

    /**
     * @return bool
     **/
    bool simulate(float step) override;

    /**
     **/
    bool render() override;

    void handleSource(const v3d::event::Source& source);

 protected:
    /**
     * @return bool
     **/
    bool start() override;

    /**
     * @return bool
     **/
    bool release() override;

 private:
    /**
     * Change the game's mode from the menu, and start a new round. Only coop differs from the
     * other modes: outside coop, the computer plays the right paddle.
     **/
    void mode(bool coop);

    /**
     * Bind a paddle command to the key an Options menu item captured. The engine saves the
     * binding, so the next run starts with it.
     **/
    void rebindPaddleKey(const v3d::event::Event& event);

    boost::shared_ptr<v3d::audio::Engine> soundEngine_;

    boost::shared_ptr<PongScene> scene_;
    boost::shared_ptr<PongRenderer> renderer_;
    // the ui, the menu, the screen and the statistics overlay
    v3d::shell::Shell shell_;
    // disconnects this object's delegate from the dispatcher when this object is destroyed
    entt::scoped_connection sources_;
};
