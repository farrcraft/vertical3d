/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::font {

/**
 * The weight and style a face is chosen by. The weight runs from 100 for thin to 900 for
 * black, with 400 regular and 700 bold.
 **/
struct FaceStyle final {
    /**
     * The weight of a regular face.
     **/
    static constexpr unsigned int REGULAR = 400;

    unsigned int weight = REGULAR;
    bool italic = false;
};

};  // namespace v3d::font
