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
 * What a walk does with each child of a node: an expression, a statement, or either.
 **/
typedef std::function<void(const ExpressionPtr &)> ExpressionVisitor;
typedef std::function<void(const StatementPtr &)> StatementVisitor;

/**
 * Hand each operand of an expression to a visitor, in source order. A leaf has none.
 *
 * This is the one place that knows which members of which node are its children, so a walk
 * that treats every kind of node alike - gathering what a tree calls, joining what it reads -
 * is a case for the kinds it cares about and this for the rest, and a new kind of node is
 * one case here rather than one in every walk.
 **/
void forEachChild(const Expression & node, const ExpressionVisitor & visit);

/**
 * Hand each expression and each statement a statement holds to the visitor for its kind,
 * in source order. A missing child - a `for` with no step, an `if` with no `else` - is
 * skipped rather than visited as null.
 **/
void forEachChild(const Statement & node, const ExpressionVisitor & expressions, const StatementVisitor & statements);

};  // namespace v3d::render::offline::sl::syntax
