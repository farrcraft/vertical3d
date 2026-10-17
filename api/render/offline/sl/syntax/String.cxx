/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "String.h"

#include <string>

namespace v3d::render::offline::sl::syntax {

String::String(const std::string & value, const v3d::render::offline::Position & at) :
    Expression(Kind::STRING, at),
    value(value) {
}

};  // namespace v3d::render::offline::sl::syntax
