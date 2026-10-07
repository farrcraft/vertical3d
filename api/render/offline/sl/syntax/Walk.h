/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <functional>

#include "Expression.h"
#include "Statement.h"

namespace v3d::render::offline::sl::syntax {

/**
 * What a tree traversal does with each child of a node: an expression, a statement, or either.
 **/
typedef std::function<void(const ExpressionPtr &)> ExpressionVisitor;
typedef std::function<void(const StatementPtr &)> StatementVisitor;

/**
 * Hand each operand of an expression to a visitor, in source order. A leaf has none.
 *
 * This is the only place that lists which members of each kind of node are its children. A
 * traversal that treats most nodes alike, such as gathering the names a tree calls, handles
 * the kinds it needs and calls this for the rest. A new kind of node then needs one case
 * here rather than one in every traversal.
 **/
void forEachChild(const Expression & node, const ExpressionVisitor & visit);

/**
 * Hand each expression and each statement a statement holds to the visitor for its kind,
 * in source order. A missing child - a `for` with no step, an `if` with no `else` - is
 * skipped rather than visited as null.
 **/
void forEachChild(const Statement & node, const ExpressionVisitor & expressions, const StatementVisitor & statements);

};  // namespace v3d::render::offline::sl::syntax
