/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Position.h>
#include <api/render/offline/sl/Types.h>

#include <cstdint>

#include <boost/shared_ptr.hpp>

namespace v3d::render::offline::sl::syntax {

/**
 * A node of the syntax tree.
 *
 * The nodes are plain data for the compiler to traverse, so their members are public and
 * there are no accessors, as in `ParameterList::Parameter`. A consumer reads `kind` and
 * casts to the class it names.
 **/
class Expression {
 public:
    enum class Kind : std::uint8_t {
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

    Expression(Kind kind, const v3d::render::offline::Position & at);
    virtual ~Expression();

    Kind kind;
    unsigned int line;
    unsigned int column;

    /**
     * What the expression turns out to be, filled in by Compiler.
     *
     * The compiler writes its analysis onto the tree itself, not into a separate map keyed
     * by node. Until the compiler has run, `storage` is UNSPECIFIED and `type` means nothing.
     **/
    Type type = Type::FLOAT;
    Storage storage = Storage::UNSPECIFIED;
};

typedef boost::shared_ptr<Expression> ExpressionPtr;

};  // namespace v3d::render::offline::sl::syntax
