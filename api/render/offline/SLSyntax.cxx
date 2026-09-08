/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SLSyntax.h"

#include <string>

namespace v3d::render::offline {

const char* name(SLType type) {
    switch (type) {
        case SLType::VOID: return "void";
        case SLType::FLOAT: return "float";
        case SLType::POINT: return "point";
        case SLType::VECTOR: return "vector";
        case SLType::NORMAL: return "normal";
        case SLType::COLOR: return "color";
        case SLType::MATRIX: return "matrix";
        case SLType::STRING: return "string";
    }
    return "";
}

const char* name(SLShaderType type) {
    switch (type) {
        case SLShaderType::SURFACE: return "surface";
        case SLShaderType::LIGHT: return "light";
        case SLShaderType::DISPLACEMENT: return "displacement";
        case SLShaderType::VOLUME: return "volume";
        case SLShaderType::IMAGER: return "imager";
    }
    return "";
}

SLExpression::SLExpression(Kind kind, unsigned int line, unsigned int column) :
    kind(kind),
    line(line),
    column(column) {
}

SLExpression::~SLExpression() {
}

SLNumber::SLNumber(float value, unsigned int line, unsigned int column) :
    SLExpression(Kind::NUMBER, line, column),
    value(value) {
}

SLString::SLString(const std::string & value, unsigned int line, unsigned int column) :
    SLExpression(Kind::STRING, line, column),
    value(value) {
}

SLVariable::SLVariable(const std::string & name, unsigned int line, unsigned int column) :
    SLExpression(Kind::VARIABLE, line, column),
    name(name) {
}

SLCall::SLCall(const std::string & name, unsigned int line, unsigned int column) :
    SLExpression(Kind::CALL, line, column),
    name(name) {
}

SLUnary::SLUnary(const std::string & op, const SLExpressionPtr & operand, unsigned int line, unsigned int column) :
    SLExpression(Kind::UNARY, line, column),
    op(op),
    operand(operand) {
}

SLBinary::SLBinary(const std::string & op, const SLExpressionPtr & left, const SLExpressionPtr & right,
    unsigned int line, unsigned int column) :
    SLExpression(Kind::BINARY, line, column),
    op(op),
    left(left),
    right(right) {
}

SLTernary::SLTernary(const SLExpressionPtr & condition, const SLExpressionPtr & whenTrue,
    const SLExpressionPtr & whenFalse, unsigned int line, unsigned int column) :
    SLExpression(Kind::TERNARY, line, column),
    condition(condition),
    whenTrue(whenTrue),
    whenFalse(whenFalse) {
}

SLCast::SLCast(SLType type, const std::string & space, const SLExpressionPtr & operand,
    unsigned int line, unsigned int column) :
    SLExpression(Kind::CAST, line, column),
    type(type),
    space(space),
    operand(operand) {
}

SLTuple::SLTuple(unsigned int line, unsigned int column) :
    SLExpression(Kind::TUPLE, line, column) {
}

SLIndex::SLIndex(const SLExpressionPtr & array, const SLExpressionPtr & index,
    unsigned int line, unsigned int column) :
    SLExpression(Kind::INDEX, line, column),
    array(array),
    index(index) {
}

SLStatement::SLStatement(Kind kind, unsigned int line, unsigned int column) :
    kind(kind),
    line(line),
    column(column) {
}

SLStatement::~SLStatement() {
}

SLBlock::SLBlock(unsigned int line, unsigned int column) :
    SLStatement(Kind::BLOCK, line, column) {
}

SLDeclaration::SLDeclaration(SLStorage storage, SLType type, unsigned int line, unsigned int column) :
    SLStatement(Kind::DECLARATION, line, column),
    storage(storage),
    type(type) {
}

SLAssignment::SLAssignment(const std::string & op, const SLExpressionPtr & target,
    const SLExpressionPtr & value, unsigned int line, unsigned int column) :
    SLStatement(Kind::ASSIGNMENT, line, column),
    op(op),
    target(target),
    value(value) {
}

SLConditional::SLConditional(const SLExpressionPtr & condition, const SLStatementPtr & whenTrue,
    const SLStatementPtr & whenFalse, unsigned int line, unsigned int column) :
    SLStatement(Kind::CONDITIONAL, line, column),
    condition(condition),
    whenTrue(whenTrue),
    whenFalse(whenFalse) {
}

SLWhile::SLWhile(const SLExpressionPtr & condition, const SLStatementPtr & body,
    unsigned int line, unsigned int column) :
    SLStatement(Kind::WHILE, line, column),
    condition(condition),
    body(body) {
}

SLFor::SLFor(unsigned int line, unsigned int column) :
    SLStatement(Kind::FOR, line, column) {
}

SLJump::SLJump(Where where, const SLExpressionPtr & value, unsigned int line, unsigned int column) :
    SLStatement(Kind::JUMP, line, column),
    where(where),
    value(value) {
}

SLExpressionStatement::SLExpressionStatement(const SLExpressionPtr & expression,
    unsigned int line, unsigned int column) :
    SLStatement(Kind::EXPRESSION, line, column),
    expression(expression) {
}

SLLighting::SLLighting(Construct construct, unsigned int line, unsigned int column) :
    SLStatement(Kind::LIGHTING, line, column),
    construct(construct) {
}

bool SLShader::supported() const {
    // displacement reaches back into dicing and is a geometry change wearing a shading
    // change's clothes; a volume shader has no place to run until there is a volume
    return type == SLShaderType::SURFACE || type == SLShaderType::LIGHT || type == SLShaderType::IMAGER;
}

};  // namespace v3d::render::offline
