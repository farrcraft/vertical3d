/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>
#include <api/type/camera/Isometric.h>

#include <functional>
#include <string>
#include <string_view>

#include <boost/shared_ptr.hpp>

#include <entt/entt.hpp>

namespace v3d::engine {

class Engine;

/**
 * The commands an IsometricController answers to, each as "context::name".
 **/
struct IsometricCommands final {
    std::string rotateLeft = "camera::rotate_left";    /**< one step clockwise, seen from above **/
    std::string rotateRight = "camera::rotate_right";  /**< one step counterclockwise **/
    std::string panUp = "camera::pan_up";              /**< away from the eye, along forward() **/
    std::string panDown = "camera::pan_down";
    std::string panLeft = "camera::pan_left";
    std::string panRight = "camera::pan_right";        /**< along right() **/
    std::string zoomIn = "camera::zoom_in";
    std::string zoomOut = "camera::zoom_out";
};

/**
 * How fast an IsometricController moves the orbit while a command is held.
 **/
struct IsometricSpeeds final {
    float pan = 10.0f;   /**< world units a second **/
    float zoom = 10.0f;  /**< orthographic half height a second **/
};

/**
 * Drives a type::camera::Isometric from commands: rotation in whole steps, and pan and zoom at
 * a speed while a command is held.
 *
 * It reads commands rather than keys. The keys are bound to the commands in the app's binding
 * document, as every other command is, so a player can rebind them from settings.
 *
 * - A rotate command turns the orbit one step when it is pressed. A repeat of a held key does
 *   not, so holding the key does not spin the camera.
 * - Pan and zoom are read with held() from simulate(step), which the app's own simulate() calls.
 *   Movement is therefore measured in seconds and does not depend on the frame rate.
 * - Every move goes through the orbit's own methods, so its clamps and its wrap apply, and a pan
 *   follows the view's axes after a rotate.
 *
 * The listener is disconnected when the controller is destroyed, so one released in release()
 * leaves nothing on the dispatcher.
 **/
class IsometricController final {
 public:
    using Commands = IsometricCommands;
    using Speeds = IsometricSpeeds;

    /**
     * Whether a command is held. Engine::held() in an app; a test passes its own.
     **/
    using Held = std::function<bool(std::string_view command)>;

    /**
     * @param orbit the orbit to drive, which the app owns and which outlives this
     * @param dispatcher where the rotate commands arrive
     * @param held asked each simulate() step whether a pan or zoom command is held
     **/
    IsometricController(type::camera::Isometric* orbit, const boost::shared_ptr<entt::dispatcher>& dispatcher, Held held,
        const Commands& commands = Commands(), const Speeds& speeds = Speeds());

    /**
     * @param engine the engine whose dispatcher and held() the controller reads, which outlives
     *        this
     **/
    IsometricController(type::camera::Isometric* orbit, const Engine& engine, const Commands& commands = Commands(),
        const Speeds& speeds = Speeds());

    IsometricController(const IsometricController&) = delete;
    IsometricController& operator=(const IsometricController&) = delete;

    /**
     * Pan and zoom by whatever is held, for one simulation step.
     *
     * @param step the step's length in seconds
     **/
    void simulate(float step);

    /**
     * Turn the orbit for a rotate command. Connected to the dispatcher by the constructor.
     **/
    void receive(const event::Event& event);

 private:
    type::camera::Isometric* orbit_;
    Held held_;
    Commands commands_;
    Speeds speeds_;
    /**< disconnects when the controller goes **/
    entt::scoped_connection listening_;
};

};  // namespace v3d::engine
