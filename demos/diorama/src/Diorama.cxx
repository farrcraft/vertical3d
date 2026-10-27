/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Diorama.h"

#include <api/audio/Loaders.h>
#include <api/audio/Play.h>
#include <api/audio/Player.h>
#include <api/engine/Engine.h>
#include <api/engine/IsometricController.h>
#include <api/input/MouseState.h>
#include <api/type/camera/Isometric.h>
#include <api/type/camera/Profile.h>
#include <api/ui/shell/GameMenu.h>
#include <api/window/Window.h>
#include <demos/diorama/src/Layout.h>
#include <demos/diorama/src/Renderer.h>

#include <string>

#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <glm/vec3.hpp>

namespace diorama {

namespace {

/** The clip the well plays, as sounds.json files it. **/
const char* const WELL_CLIP = "well";

/** Where the voice sits above the well's base: at the water. **/
constexpr glm::vec3 WATER(0.0F, 0.6F, 0.0F);

/** How long the well's voice takes to fade up, in milliseconds. **/
constexpr int FADE_IN = 1000;

/** The orthographic half height the camera starts at, which shows the whole village. **/
constexpr float START_ZOOM = 8.0F;

};  // namespace

/**
 **/
Diorama::Diorama(const std::string& appPath) :
    v3d::engine::Engine(appPath),
    shell_(this, { .menu = true, .statistics = "diorama::toggleStatistics" }) {
    // the pans and zooms are read while held, and the rotates reach the controller through the
    // dispatcher, so each is registered with no handler of its own
    const v3d::engine::IsometricController::Commands camera;
    for (const std::string& command : { camera.rotateLeft, camera.rotateRight, camera.panUp, camera.panDown,
        camera.panLeft, camera.panRight, camera.zoomIn, camera.zoomOut }) {
        commands().addHeld(command);
    }
}

/**
 **/
bool Diorama::start() {
    window()->caption("Diorama");

    sound_ = boost::make_shared<v3d::audio::Player>(logger(), dispatcher());
    v3d::audio::registerLoaders(*assets(), logger());
    // the return is ignored. When no device opens, the player logs why and stays silent
    sound_->initialize();
    if (const boost::json::object* sounds = document(v3d::audio::Player::DOCUMENT)) {
        sound_->load(*sounds, *assets());
    }

    const v3d::ui::shell::GameMenu::Suspend pause = [this](bool suspended) { paused_ = suspended; };
    if (!shell_.start(pause)) {
        return false;
    }

    orbit_.start(v3d::type::camera::Isometric::DIAGONAL_START);
    orbit_.zoom(START_ZOOM);
    controller_ = boost::make_shared<v3d::engine::IsometricController>(&orbit_, *this);

    renderer_ = boost::make_shared<Renderer>(window(), logger(), assets(), &shell_);
    if (!renderer_->place(&registry(), layout())) {
        return false;
    }

    v3d::audio::Play how;
    how.bus = "ambience";
    how.loops = -1;
    how.fadeInMs = FADE_IN;
    how.position = WELL + WATER;
    if (sound_->play(WELL_CLIP, how) == v3d::audio::Voice::None) {
        // a device that did not open has already said so, and the diorama runs silent
        logger()->get()->warn("the well is silent");
    }
    return true;
}

/**
 **/
bool Diorama::tick(unsigned int delta) {
    if (!v3d::engine::Engine::tick(delta)) {
        return false;
    }
    const v3d::input::MouseState* pointer = mouse();
    if (pointer != nullptr && !paused_) {
        controller_->wheel(pointer->wheel());
    }
    // the camera of the last frame drawn, which is at most a frame behind
    const v3d::type::camera::Profile& view = renderer_->camera().profile();
    sound_->listener({.position = view.eye(), .forward = view.direction(), .up = view.up(), .right = view.right()});
    return true;
}

/**
 **/
bool Diorama::simulate(float step) {
    if (!v3d::engine::Engine::simulate(step)) {
        return false;
    }
    if (paused_) {
        return true;
    }
    controller_->simulate(step);
    time_ += step;
    if (time_ >= DAY) {
        time_ -= DAY;
    }
    return true;
}

/**
 **/
bool Diorama::render() {
    renderer_->draw(orbit_, sunlight(time_), alpha(), registry());
    return true;
}

/**
 **/
bool Diorama::release() {
    controller_.reset();
    if (sound_) {
        // the engine quits SDL as soon as release() returns, and the player's tracks have to go
        // before the audio subsystem does
        sound_->shutdown();
    }
    // the screen draws through the renderer, so it goes first
    shell_.release();
    if (renderer_) {
        // the device has to be idle before the window it presents to is destroyed
        renderer_->shutdown();
    }
    return true;
}

};  // namespace diorama
