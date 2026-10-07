/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Walk.h"

#include "Assignment.h"
#include "Binary.h"
#include "Block.h"
#include "Call.h"
#include "Cast.h"
#include "Conditional.h"
#include "Declaration.h"
#include "Declarator.h"
#include "ExpressionStatement.h"
#include "For.h"
#include "Index.h"
#include "Jump.h"
#include "Lighting.h"
#include "Ternary.h"
#include "Tuple.h"
#include "Unary.h"
#include "While.h"

namespace v3d::render::offline::sl::syntax {

namespace {

void expression(const ExpressionPtr & child, const ExpressionVisitor & visit) {
    if (child) {
        visit(child);
    }
}

void statement(const StatementPtr & child, const StatementVisitor & visit) {
    if (child) {
        visit(child);
    }
}

};  // namespace

void forEachChild(const Expression & node, const ExpressionVisitor & visit) {
    switch (node.kind) {
        case Expression::Kind::CALL:
            for (const ExpressionPtr & argument : static_cast<const Call &>(node).arguments) {
                expression(argument, visit);
            }
            return;
        case Expression::Kind::UNARY:
            expression(static_cast<const Unary &>(node).operand, visit);
            return;
        case Expression::Kind::BINARY: {
            const Binary & binary = static_cast<const Binary &>(node);
            expression(binary.left, visit);
            expression(binary.right, visit);
            return;
        }
        case Expression::Kind::TERNARY: {
            const Ternary & ternary = static_cast<const Ternary &>(node);
            expression(ternary.condition, visit);
            expression(ternary.whenTrue, visit);
            expression(ternary.whenFalse, visit);
            return;
        }
        case Expression::Kind::CAST:
            expression(static_cast<const Cast &>(node).operand, visit);
            return;
        case Expression::Kind::TUPLE:
            for (const ExpressionPtr & element : static_cast<const Tuple &>(node).elements) {
                expression(element, visit);
            }
            return;
        case Expression::Kind::INDEX: {
            const Index & index = static_cast<const Index &>(node);
            expression(index.array, visit);
            expression(index.index, visit);
            return;
        }
        case Expression::Kind::NUMBER:
        case Expression::Kind::STRING:
        case Expression::Kind::VARIABLE:
            return;
    }
}

void forEachChild(const Statement & node, const ExpressionVisitor & expressions, const StatementVisitor & statements) {
    switch (node.kind) {
        case Statement::Kind::BLOCK:
            for (const StatementPtr & inner : static_cast<const Block &>(node).statements) {
                statement(inner, statements);
            }
            return;
        case Statement::Kind::DECLARATION:
            for (const Declarator & declarator : static_cast<const Declaration &>(node).declarators) {
                expression(declarator.initialiser, expressions);
            }
            return;
        case Statement::Kind::ASSIGNMENT: {
            const Assignment & assignment = static_cast<const Assignment &>(node);
            expression(assignment.target, expressions);
            expression(assignment.value, expressions);
            return;
        }
        case Statement::Kind::CONDITIONAL: {
            const Conditional & conditional = static_cast<const Conditional &>(node);
            expression(conditional.condition, expressions);
            statement(conditional.whenTrue, statements);
            statement(conditional.whenFalse, statements);
            return;
        }
        case Statement::Kind::WHILE: {
            const While & loop = static_cast<const While &>(node);
            expression(loop.condition, expressions);
            statement(loop.body, statements);
            return;
        }
        case Statement::Kind::FOR: {
            const For & loop = static_cast<const For &>(node);
            statement(loop.initialiser, statements);
            expression(loop.condition, expressions);
            statement(loop.step, statements);
            statement(loop.body, statements);
            return;
        }
        case Statement::Kind::JUMP:
            expression(static_cast<const Jump &>(node).value, expressions);
            return;
        case Statement::Kind::EXPRESSION:
            expression(static_cast<const ExpressionStatement &>(node).expression, expressions);
            return;
        case Statement::Kind::LIGHTING: {
            const Lighting & lighting = static_cast<const Lighting &>(node);
            for (const ExpressionPtr & argument : lighting.arguments) {
                expression(argument, expressions);
            }
            statement(lighting.body, statements);
            return;
        }
    }
}

};  // namespace v3d::render::offline::sl::syntax
