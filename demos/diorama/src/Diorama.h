/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/audio/Player.h>
#include <api/engine/Engine.h>
#include <api/engine/IsometricController.h>
#include <api/shell/Shell.h>
#include <api/type/camera/Isometric.h>
#include <demos/diorama/src/Renderer.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace diorama {

/**
 * The diorama: a village of glTF models drawn lit, with a shadow map, under a sun that goes
 * around the sky. The camera is an isometric orbit the player turns, pans and zooms, and the well
 * plays a looping voice placed where it stands, heard from the camera.
 *
 * The menu pauses the sun and the camera's pan and zoom. A rotate command still turns the
 * camera while the menu is up, because the controller answers every rotate command that reaches
 * the dispatcher.
 **/
class Diorama final : public v3d::engine::Engine {
 public:
    explicit Diorama(const std::string& appPath);

    /**
     * Zoom by what the wheel turned, and hear the well from where the camera now is.
     **/
    bool tick(unsigned int delta) override;

    /**
     * Pan and zoom by what is held, and move the sun on.
     **/
    bool simulate(float step) override;

    bool render() override;

 protected:
    /**
     * Open the audio device, build the ui and the renderer, stand the village up, and start the
     * well's voice.
     *
     * @return false when a model cannot be loaded or the ui document does not load
     **/
    bool start() override;

    bool release() override;

 private:
    // the ui, the menu, the screen and the statistics overlay
    v3d::shell::Shell shell_;
    boost::shared_ptr<v3d::audio::Player> sound_;
    v3d::type::camera::Isometric orbit_;
    boost::shared_ptr<v3d::engine::IsometricController> controller_;
    boost::shared_ptr<Renderer> renderer_;
    float time_ = 0.0F;  /**< seconds into the sun's day **/
    bool paused_ = false;
};

};  // namespace diorama
