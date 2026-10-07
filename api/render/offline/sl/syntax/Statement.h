/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <boost/shared_ptr.hpp>

namespace v3d::render::offline::sl::syntax {

/**
 * A statement of a shader body.
 **/
class Statement {
 public:
    enum class Kind {
        BLOCK,
        DECLARATION,
        ASSIGNMENT,
        CONDITIONAL,
        WHILE,
        FOR,
        /** break, continue or return. **/
        JUMP,
        EXPRESSION,
        /** illuminance, illuminate or solar - a construct with a body rather than a call. **/
        LIGHTING
    };

    Statement(Kind kind, unsigned int line, unsigned int column);
    virtual ~Statement();

    Kind kind;
    unsigned int line;
    unsigned int column;
};

typedef boost::shared_ptr<Statement> StatementPtr;

};  // namespace v3d::render::offline::sl::syntax
