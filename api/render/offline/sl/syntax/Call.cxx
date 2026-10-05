/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Call.h"

#include <string>

namespace v3d::render::offline::sl::syntax {

Call::Call(const std::string & name, unsigned int line, unsigned int column) :
    Expression(Kind::CALL, line, column),
    name(name) {
}

};  // namespace v3d::render::offline::sl::syntax
