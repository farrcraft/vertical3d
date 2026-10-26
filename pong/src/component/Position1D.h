/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

/**
 * An entity's position along a line. A plain aggregate: snapshot() copies it, and simulation
 * writes value directly every step.
 **/
struct Position1D final {
    float value = 0.0F;
};

/**
 * The position alpha of the way from one to the other. ecs::interpolated() uses it to draw
 * the position between two simulation steps.
 **/
Position1D interpolate(const Position1D& from, const Position1D& to, float alpha);

