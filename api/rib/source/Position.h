/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

/**
 * What the RIB and shading language lexers share: character input that keeps its position, and a
 * lexical unit.
 **/
namespace v3d::rib::source {

/**
 * Where something starts in a source file, counted from 1. Zero means the position is not
 * known.
 **/
struct Position {
    unsigned int line = 0;
    unsigned int column = 0;
};

};  // namespace v3d::rib::source
