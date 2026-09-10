/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "Types.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::offline::sl {


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

class Number final : public Expression {
 public:
    Number(float value, unsigned int line, unsigned int column);

    float value = 0.0f;
};

class String final : public Expression {
 public:
    String(const std::string & value, unsigned int line, unsigned int column);

    std::string value;
};

class Variable final : public Expression {
 public:
    Variable(const std::string & name, unsigned int line, unsigned int column);

    std::string name;
    /**
     * Which symbol the name resolved to, or -1 before Compiler has run. A local declared
     * twice in nested scopes is two symbols, so the index is what the machine allocates a
     * register against rather than the name.
     **/
    int symbol = -1;
};

class Call final : public Expression {
 public:
    Call(const std::string & name, unsigned int line, unsigned int column);

    std::string name;
    std::vector<ExpressionPtr> arguments;
    /**
     * Which of the shader's own functions this calls, or -1 for one of the standard
     * library's. Filled in by Compiler.
     **/
    int function = -1;
    /**
     * Which signature of the standard library it matched, as an index into `builtins()`,
     * or -1. An index rather than the signature itself, because the signature knows about
     * types and this header is what types are declared in.
     **/
    int signature = -1;
};

/**
 * Negation or logical not, the two prefix operators the language has.
 **/
class Unary final : public Expression {
 public:
    Unary(const std::string & op, const ExpressionPtr & operand, unsigned int line, unsigned int column);

    std::string op;
    ExpressionPtr operand;
};

/**
 * Everything infix, including the two that read as something else: '.' is a dot product
 * rather than a member access, and '^' is a cross product rather than an exponent.
 **/
class Binary final : public Expression {
 public:
    Binary(const std::string & op, const ExpressionPtr & left, const ExpressionPtr & right,
        unsigned int line, unsigned int column);

    std::string op;
    ExpressionPtr left;
    ExpressionPtr right;
};

class Ternary final : public Expression {
 public:
    Ternary(const ExpressionPtr & condition, const ExpressionPtr & whenTrue,
        const ExpressionPtr & whenFalse, unsigned int line, unsigned int column);

    ExpressionPtr condition;
    ExpressionPtr whenTrue;
    ExpressionPtr whenFalse;
};

/**
 * A typecast, which is also how a scene names a coordinate space:
 * `point "world" (0, 0, 0)` casts and transforms in one. The space is empty when none was
 * given, which means the shader's current space.
 **/
class Cast final : public Expression {
 public:
    Cast(Type type, const std::string & space, const ExpressionPtr & operand,
        unsigned int line, unsigned int column);

    Type type = Type::FLOAT;
    std::string space;
    ExpressionPtr operand;
};

class Tuple final : public Expression {
 public:
    Tuple(unsigned int line, unsigned int column);

    std::vector<ExpressionPtr> elements;
};

class Index final : public Expression {
 public:
    Index(const ExpressionPtr & array, const ExpressionPtr & index, unsigned int line, unsigned int column);

    ExpressionPtr array;
    ExpressionPtr index;
};

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

class Block final : public Statement {
 public:
    Block(unsigned int line, unsigned int column);

    std::vector<StatementPtr> statements;
};

typedef boost::shared_ptr<Block> BlockPtr;

/**
 * One name of a declaration, with the expression that initialises it or none.
 **/
class Declarator final {
 public:
    std::string name;
    ExpressionPtr initialiser;
    unsigned int line = 0;
    unsigned int column = 0;
    /** The symbol Compiler gave it, or -1 before that pass has run. **/
    int symbol = -1;
};

class Declaration final : public Statement {
 public:
    Declaration(Storage storage, Type type, unsigned int line, unsigned int column);

    Storage storage = Storage::UNSPECIFIED;
    Type type = Type::FLOAT;
    std::vector<Declarator> declarators;
};

/**
 * An assignment and its compound forms. The target is an expression rather than a name so
 * that `Ci[0] = 1` parses; whether it is something that can be assigned to is the
 * compiler's answer.
 **/
class Assignment final : public Statement {
 public:
    Assignment(const std::string & op, const ExpressionPtr & target, const ExpressionPtr & value,
        unsigned int line, unsigned int column);

    std::string op;
    ExpressionPtr target;
    ExpressionPtr value;
};

class Conditional final : public Statement {
 public:
    Conditional(const ExpressionPtr & condition, const StatementPtr & whenTrue,
        const StatementPtr & whenFalse, unsigned int line, unsigned int column);

    ExpressionPtr condition;
    StatementPtr whenTrue;
    /** Null when there was no else. **/
    StatementPtr whenFalse;
};

class While final : public Statement {
 public:
    While(const ExpressionPtr & condition, const StatementPtr & body, unsigned int line, unsigned int column);

    ExpressionPtr condition;
    StatementPtr body;
};

class For final : public Statement {
 public:
    For(unsigned int line, unsigned int column);

    /** Any of the three heads may be null, which is what an empty one in the source is. **/
    StatementPtr initialiser;
    ExpressionPtr condition;
    StatementPtr step;
    StatementPtr body;
};

class Jump final : public Statement {
 public:
    enum class Where {
        BREAK,
        CONTINUE,
        RETURN
    };

    Jump(Where where, const ExpressionPtr & value, unsigned int line, unsigned int column);

    Where where = Where::RETURN;
    /** What a return returns, or null for a bare one and for the other two. **/
    ExpressionPtr value;
};

class ExpressionStatement final : public Statement {
 public:
    ExpressionStatement(const ExpressionPtr & expression, unsigned int line, unsigned int column);

    ExpressionPtr expression;
};

/**
 * The three message passing constructs. Each takes a parenthesised argument list and a body
 * that runs once per light, or once per surface point being lit.
 **/
class Lighting final : public Statement {
 public:
    enum class Construct {
        ILLUMINANCE,
        ILLUMINATE,
        SOLAR
    };

    Lighting(Construct construct, unsigned int line, unsigned int column);

    Construct construct = Construct::ILLUMINANCE;
    std::vector<ExpressionPtr> arguments;
    StatementPtr body;
};

/**
 * A formal parameter of a shader or of a function inside one.
 *
 * A shader parameter has a **required** default - SL has no uninitialised parameter, and
 * the default is what a scene that does not mention it gets. A function's formals have
 * none, which is the one way the two lists differ.
 **/
class Parameter final {
 public:
    Storage storage = Storage::UNSPECIFIED;
    /** Whether the shader writes it back, which only a shader parameter may be. **/
    bool output = false;
    Type type = Type::FLOAT;
    std::string name;
    ExpressionPtr defaultValue;
    unsigned int line = 0;
    unsigned int column = 0;
    /** The symbol Compiler gave it, or -1 before that pass has run. **/
    int symbol = -1;
};

/**
 * A function defined inside a shader.
 *
 * Recursion is rejected at the call graph rather than here: the machine has a register file
 * per shader run and no call stack, so a recursive shader has no meaning to give.
 **/
class Function final {
 public:
    Type type = Type::VOID;
    std::string name;
    std::vector<Parameter> parameters;
    BlockPtr body;
    unsigned int line = 0;
    unsigned int column = 0;
};

class Shader final {
 public:
    /**
     * Whether this phase executes a shader of this type. A displacement or a volume shader
     * parses and is reported by name, which is the difference between a scene naming
     * something unsupported and a scene that is malformed.
     **/
    bool supported() const;

    ShaderType type = ShaderType::SURFACE;
    std::string name;
    std::vector<Parameter> parameters;
    std::vector<Function> functions;
    BlockPtr body;
    unsigned int line = 0;
    unsigned int column = 0;
};

typedef boost::shared_ptr<Shader> ShaderPtr;

};  // namespace v3d::render::offline::sl
