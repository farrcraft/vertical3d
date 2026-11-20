/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::engine {

/**
 * How fast an IsometricController moves the orbit while a command is held.
 **/
struct IsometricSpeeds final {
    float pan = 10.0F;   /**< world units a second **/
    float zoom = 10.0F;  /**< orthographic half height a second **/
    /** orthographic half height a notch of the wheel zooms by. A notch away from the player
     * zooms in. A negative value turns the wheel round, for a player who prefers it **/
    float notch = 1.0F;
};

};  // namespace v3d::engine
