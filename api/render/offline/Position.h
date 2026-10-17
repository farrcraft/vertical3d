/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::render::offline {

/**
 * Where something starts in a source file, counted from 1. Zero means the position is not
 * known.
 **/
struct Position {
    unsigned int line = 0;
    unsigned int column = 0;
};

};  // namespace v3d::render::offline
