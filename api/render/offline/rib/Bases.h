/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Requests.h"

namespace v3d::render::offline::rib {

/**
 * The basis matrix RI defines for a name: "bezier", "b-spline", "catmull-rom", "hermite" or
 * "power".
 *
 * @return whether the name is one of them
 **/
bool namedBasis(const std::string & name, Basis::Matrix * matrix);

/**
 * @return the name RI gives a basis matrix, or empty for a matrix that is not one of its own
 **/
std::string basisName(const Basis::Matrix & matrix);

};  // namespace v3d::render::offline::rib
