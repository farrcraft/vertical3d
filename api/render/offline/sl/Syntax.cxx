/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Syntax.h"

#include <string>

namespace v3d::render::offline::sl {

Expression::Expression(Kind kind, unsigned int line, unsigned int column) :
    kind(kind),
    line(line),
    column(column) {
}

Expression::~Expression() {
}

Number::Number(float value, unsigned int line, unsigned int column) :
    Expression(Kind::NUMBER, line, column),
    value(value) {
}

String::String(const std::string & value, unsigned int line, unsigned int column) :
    Expression(Kind::STRING, line, column),
    value(value) {
}

Variable::Variable(const std::string & name, unsigned int line, unsigned int column) :
    Expression(Kind::VARIABLE, line, column),
    name(name) {
}

Call::Call(const std::string & name, unsigned int line, unsigned int column) :
    Expression(Kind::CALL, line, column),
    name(name) {
}

Unary::Unary(const std::string & op, const ExpressionPtr & operand, unsigned int line, unsigned int column) :
    Expression(Kind::UNARY, line, column),
    op(op),
    operand(operand) {
}

Binary::Binary(const std::string & op, const ExpressionPtr & left, const ExpressionPtr & right,
    unsigned int line, unsigned int column) :
    Expression(Kind::BINARY, line, column),
    op(op),
    left(left),
    right(right) {
}

Ternary::Ternary(const ExpressionPtr & condition, const ExpressionPtr & whenTrue,
    const ExpressionPtr & whenFalse, unsigned int line, unsigned int column) :
    Expression(Kind::TERNARY, line, column),
    condition(condition),
    whenTrue(whenTrue),
    whenFalse(whenFalse) {
}

Cast::Cast(Type type, const std::string & space, const ExpressionPtr & operand,
    unsigned int line, unsigned int column) :
    Expression(Kind::CAST, line, column),
    type(type),
    space(space),
    operand(operand) {
}

Tuple::Tuple(unsigned int line, unsigned int column) :
    Expression(Kind::TUPLE, line, column) {
}

Index::Index(const ExpressionPtr & array, const ExpressionPtr & index,
    unsigned int line, unsigned int column) :
    Expression(Kind::INDEX, line, column),
    array(array),
    index(index) {
}

Statement::Statement(Kind kind, unsigned int line, unsigned int column) :
    kind(kind),
    line(line),
    column(column) {
}

Statement::~Statement() {
}

Block::Block(unsigned int line, unsigned int column) :
    Statement(Kind::BLOCK, line, column) {
}

Declaration::Declaration(Storage storage, Type type, unsigned int line, unsigned int column) :
    Statement(Kind::DECLARATION, line, column),
    storage(storage),
    type(type) {
}

Assignment::Assignment(const std::string & op, const ExpressionPtr & target,
    const ExpressionPtr & value, unsigned int line, unsigned int column) :
    Statement(Kind::ASSIGNMENT, line, column),
    op(op),
    target(target),
    value(value) {
}

Conditional::Conditional(const ExpressionPtr & condition, const StatementPtr & whenTrue,
    const StatementPtr & whenFalse, unsigned int line, unsigned int column) :
    Statement(Kind::CONDITIONAL, line, column),
    condition(condition),
    whenTrue(whenTrue),
    whenFalse(whenFalse) {
}

While::While(const ExpressionPtr & condition, const StatementPtr & body,
    unsigned int line, unsigned int column) :
    Statement(Kind::WHILE, line, column),
    condition(condition),
    body(body) {
}

For::For(unsigned int line, unsigned int column) :
    Statement(Kind::FOR, line, column) {
}

Jump::Jump(Where where, const ExpressionPtr & value, unsigned int line, unsigned int column) :
    Statement(Kind::JUMP, line, column),
    where(where),
    value(value) {
}

ExpressionStatement::ExpressionStatement(const ExpressionPtr & expression,
    unsigned int line, unsigned int column) :
    Statement(Kind::EXPRESSION, line, column),
    expression(expression) {
}

Lighting::Lighting(Construct construct, unsigned int line, unsigned int column) :
    Statement(Kind::LIGHTING, line, column),
    construct(construct) {
}

bool Shader::supported() const {
    // displacement reaches back into dicing and is a geometry change wearing a shading
    // change's clothes; a volume shader has no place to run until there is a volume
    return type == ShaderType::SURFACE || type == ShaderType::LIGHT || type == ShaderType::IMAGER;
}

};  // namespace v3d::render::offline::sl
