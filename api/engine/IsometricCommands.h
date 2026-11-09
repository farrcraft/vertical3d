/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

namespace v3d::engine {

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

};  // namespace v3d::engine
