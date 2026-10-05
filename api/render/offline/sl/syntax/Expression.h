/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Types.h>

#include <boost/shared_ptr.hpp>

namespace v3d::render::offline::sl::syntax {

/**
 * A node of the syntax tree.
 *
 * The nodes are data the compiler walks rather than objects with behaviour, so their
 * members are public and there are no accessors - the same shape `ParameterList::Parameter`
 * already has. A consumer reads `kind` and casts to the class it names.
 **/
class Expression {
 public:
    enum class Kind {
        NUMBER,
        STRING,
        VARIABLE,
        CALL,
        UNARY,
        BINARY,
        TERNARY,
        /** A typecast, with the optional space name that makes it a transform. **/
        CAST,
        /** A parenthesised list: three floats are a point or a colour, sixteen a matrix. **/
        TUPLE,
        INDEX
    };

    Expression(Kind kind, unsigned int line, unsigned int column);
    virtual ~Expression();

    Kind kind;
    unsigned int line;
    unsigned int column;

    /**
     * What the expression turns out to be, filled in by Compiler.
     *
     * The tree carries its own analysis rather than a second structure keyed by node: the
     * two are written and read by passes in the same library, and a parallel map would be
     * one more thing to keep in step. Until the compiler has run, `storage` is UNSPECIFIED
     * and `type` means nothing.
     **/
    Type type = Type::FLOAT;
    Storage storage = Storage::UNSPECIFIED;
};

typedef boost::shared_ptr<Expression> ExpressionPtr;

};  // namespace v3d::render::offline::sl::syntax
