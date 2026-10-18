/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Variable.h"

#include <api/render/offline/Position.h>

#include "Expression.h"

#include <string>

namespace v3d::render::offline::sl::syntax {

Variable::Variable(const std::string & name, const v3d::render::offline::Position & at) :
    Expression(Kind::VARIABLE, at),
    name(name) {
}

};  // namespace v3d::render::offline::sl::syntax
