/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <stdexcept>

namespace v3d::moya::lpe {

/** A set of expressions whose automaton holds more than kMostStates states. **/
class Refused : public std::runtime_error {
 public:
    using std::runtime_error::runtime_error;
};

};  // namespace v3d::moya::lpe
