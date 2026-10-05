/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Index.h"

namespace v3d::render::offline::sl::syntax {

Index::Index(const ExpressionPtr & array, const ExpressionPtr & index,
    unsigned int line, unsigned int column) :
    Expression(Kind::INDEX, line, column),
    array(array),
    index(index) {
}

};  // namespace v3d::render::offline::sl::syntax
