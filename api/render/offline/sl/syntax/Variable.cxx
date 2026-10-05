/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Variable.h"

#include <string>

namespace v3d::render::offline::sl::syntax {

Variable::Variable(const std::string & name, unsigned int line, unsigned int column) :
    Expression(Kind::VARIABLE, line, column),
    name(name) {
}

};  // namespace v3d::render::offline::sl::syntax
