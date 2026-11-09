/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::ui::immediate {

/**
 * Where the next widget goes, and what the row being written has come to.
 **/
struct Row final {
    Row() noexcept;

    float margin;     /**< where a new row starts **/
    float right;      /**< where the room a widget may take ends **/
    float penY;       /**< where the next row starts **/
    float top;        /**< the top of the row being written **/
    float height;     /**< the tallest thing on it so far **/
    float lastRight;  /**< just past the last widget, which sameLine() starts from **/
    bool sameLine;
};

};  // namespace v3d::ui::immediate
