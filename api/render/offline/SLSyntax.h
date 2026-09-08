/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::offline {

/**
 * A value's type. The three point-like ones are all three floats and differ only in how a
 * transform treats them - a point translates, a vector does not, and a normal goes by the
 * inverse transpose - which is the compiler's problem rather than the parser's.
 *
 * SL has no integer type: a count, an index and a colour component are all floats.
 **/
enum class SLType {
    VOID,
    FLOAT,
    POINT,
    VECTOR,
    NORMAL,
    COLOR,
    MATRIX,
    STRING
};

/**
 * Whether a value is stored once or once per shading point. UNSPECIFIED is what a
 * declaration that named neither carries; the varying inference in the compiler is what
 * turns it into one of the other two.
 **/
enum class SLStorage {
    UNSPECIFIED,
    UNIFORM,
    VARYING
};

/**
 * The five shader types. All five parse; surface, light and imager are the three this
 * phase executes.
 **/
enum class SLShaderType {
    SURFACE,
    LIGHT,
    DISPLACEMENT,
    VOLUME,
    IMAGER
};

/**
 * The name a type is written with, for a diagnostic.
 **/
const char* name(SLType type);
const char* name(SLShaderType type);

/**
 * A node of the syntax tree.
 *
 * The nodes are data the compiler walks rather than objects with behaviour, so their
 * members are public and there are no accessors - the same shape `ParameterList::Parameter`
 * already has. A consumer reads `kind` and casts to the class it names.
 **/
class SLExpression {
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

    SLExpression(Kind kind, unsigned int line, unsigned int column);
    virtual ~SLExpression();

    Kind kind;
    unsigned int line;
    unsigned int column;

    /**
     * What the expression turns out to be, filled in by SLCompiler.
     *
     * The tree carries its own analysis rather than a second structure keyed by node: the
     * two are written and read by passes in the same library, and a parallel map would be
     * one more thing to keep in step. Until the compiler has run, `storage` is UNSPECIFIED
     * and `type` means nothing.
     **/
    SLType type = SLType::FLOAT;
    SLStorage storage = SLStorage::UNSPECIFIED;
};

typedef boost::shared_ptr<SLExpression> SLExpressionPtr;

class SLNumber final : public SLExpression {
 public:
    SLNumber(float value, unsigned int line, unsigned int column);

    float value = 0.0f;
};

class SLString final : public SLExpression {
 public:
    SLString(const std::string & value, unsigned int line, unsigned int column);

    std::string value;
};

class SLVariable final : public SLExpression {
 public:
    SLVariable(const std::string & name, unsigned int line, unsigned int column);

    std::string name;
    /**
     * Which symbol the name resolved to, or -1 before SLCompiler has run. A local declared
     * twice in nested scopes is two symbols, so the index is what the machine allocates a
     * register against rather than the name.
     **/
    int symbol = -1;
};

class SLCall final : public SLExpression {
 public:
    SLCall(const std::string & name, unsigned int line, unsigned int column);

    std::string name;
    std::vector<SLExpressionPtr> arguments;
    /**
     * Which of the shader's own functions this calls, or -1 for one of the standard
     * library's. Filled in by SLCompiler.
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
class SLUnary final : public SLExpression {
 public:
    SLUnary(const std::string & op, const SLExpressionPtr & operand, unsigned int line, unsigned int column);

    std::string op;
    SLExpressionPtr operand;
};

/**
 * Everything infix, including the two that read as something else: '.' is a dot product
 * rather than a member access, and '^' is a cross product rather than an exponent.
 **/
class SLBinary final : public SLExpression {
 public:
    SLBinary(const std::string & op, const SLExpressionPtr & left, const SLExpressionPtr & right,
        unsigned int line, unsigned int column);

    std::string op;
    SLExpressionPtr left;
    SLExpressionPtr right;
};

class SLTernary final : public SLExpression {
 public:
    SLTernary(const SLExpressionPtr & condition, const SLExpressionPtr & whenTrue,
        const SLExpressionPtr & whenFalse, unsigned int line, unsigned int column);

    SLExpressionPtr condition;
    SLExpressionPtr whenTrue;
    SLExpressionPtr whenFalse;
};

/**
 * A typecast, which is also how a scene names a coordinate space:
 * `point "world" (0, 0, 0)` casts and transforms in one. The space is empty when none was
 * given, which means the shader's current space.
 **/
class SLCast final : public SLExpression {
 public:
    SLCast(SLType type, const std::string & space, const SLExpressionPtr & operand,
        unsigned int line, unsigned int column);

    SLType type = SLType::FLOAT;
    std::string space;
    SLExpressionPtr operand;
};

class SLTuple final : public SLExpression {
 public:
    SLTuple(unsigned int line, unsigned int column);

    std::vector<SLExpressionPtr> elements;
};

class SLIndex final : public SLExpression {
 public:
    SLIndex(const SLExpressionPtr & array, const SLExpressionPtr & index, unsigned int line, unsigned int column);

    SLExpressionPtr array;
    SLExpressionPtr index;
};

/**
 * A statement of a shader body.
 **/
class SLStatement {
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

    SLStatement(Kind kind, unsigned int line, unsigned int column);
    virtual ~SLStatement();

    Kind kind;
    unsigned int line;
    unsigned int column;
};

typedef boost::shared_ptr<SLStatement> SLStatementPtr;

class SLBlock final : public SLStatement {
 public:
    SLBlock(unsigned int line, unsigned int column);

    std::vector<SLStatementPtr> statements;
};

typedef boost::shared_ptr<SLBlock> SLBlockPtr;

/**
 * One name of a declaration, with the expression that initialises it or none.
 **/
class SLDeclarator final {
 public:
    std::string name;
    SLExpressionPtr initialiser;
    unsigned int line = 0;
    unsigned int column = 0;
    /** The symbol SLCompiler gave it, or -1 before that pass has run. **/
    int symbol = -1;
};

class SLDeclaration final : public SLStatement {
 public:
    SLDeclaration(SLStorage storage, SLType type, unsigned int line, unsigned int column);

    SLStorage storage = SLStorage::UNSPECIFIED;
    SLType type = SLType::FLOAT;
    std::vector<SLDeclarator> declarators;
};

/**
 * An assignment and its compound forms. The target is an expression rather than a name so
 * that `Ci[0] = 1` parses; whether it is something that can be assigned to is the
 * compiler's answer.
 **/
class SLAssignment final : public SLStatement {
 public:
    SLAssignment(const std::string & op, const SLExpressionPtr & target, const SLExpressionPtr & value,
        unsigned int line, unsigned int column);

    std::string op;
    SLExpressionPtr target;
    SLExpressionPtr value;
};

class SLConditional final : public SLStatement {
 public:
    SLConditional(const SLExpressionPtr & condition, const SLStatementPtr & whenTrue,
        const SLStatementPtr & whenFalse, unsigned int line, unsigned int column);

    SLExpressionPtr condition;
    SLStatementPtr whenTrue;
    /** Null when there was no else. **/
    SLStatementPtr whenFalse;
};

class SLWhile final : public SLStatement {
 public:
    SLWhile(const SLExpressionPtr & condition, const SLStatementPtr & body, unsigned int line, unsigned int column);

    SLExpressionPtr condition;
    SLStatementPtr body;
};

class SLFor final : public SLStatement {
 public:
    SLFor(unsigned int line, unsigned int column);

    /** Any of the three heads may be null, which is what an empty one in the source is. **/
    SLStatementPtr initialiser;
    SLExpressionPtr condition;
    SLStatementPtr step;
    SLStatementPtr body;
};

class SLJump final : public SLStatement {
 public:
    enum class Where {
        BREAK,
        CONTINUE,
        RETURN
    };

    SLJump(Where where, const SLExpressionPtr & value, unsigned int line, unsigned int column);

    Where where = Where::RETURN;
    /** What a return returns, or null for a bare one and for the other two. **/
    SLExpressionPtr value;
};

class SLExpressionStatement final : public SLStatement {
 public:
    SLExpressionStatement(const SLExpressionPtr & expression, unsigned int line, unsigned int column);

    SLExpressionPtr expression;
};

/**
 * The three message passing constructs. Each takes a parenthesised argument list and a body
 * that runs once per light, or once per surface point being lit.
 **/
class SLLighting final : public SLStatement {
 public:
    enum class Construct {
        ILLUMINANCE,
        ILLUMINATE,
        SOLAR
    };

    SLLighting(Construct construct, unsigned int line, unsigned int column);

    Construct construct = Construct::ILLUMINANCE;
    std::vector<SLExpressionPtr> arguments;
    SLStatementPtr body;
};

/**
 * A formal parameter of a shader or of a function inside one.
 *
 * A shader parameter has a **required** default - SL has no uninitialised parameter, and
 * the default is what a scene that does not mention it gets. A function's formals have
 * none, which is the one way the two lists differ.
 **/
class SLParameter final {
 public:
    SLStorage storage = SLStorage::UNSPECIFIED;
    /** Whether the shader writes it back, which only a shader parameter may be. **/
    bool output = false;
    SLType type = SLType::FLOAT;
    std::string name;
    SLExpressionPtr defaultValue;
    unsigned int line = 0;
    unsigned int column = 0;
    /** The symbol SLCompiler gave it, or -1 before that pass has run. **/
    int symbol = -1;
};

/**
 * A function defined inside a shader.
 *
 * Recursion is rejected at the call graph rather than here: the machine has a register file
 * per shader run and no call stack, so a recursive shader has no meaning to give.
 **/
class SLFunction final {
 public:
    SLType type = SLType::VOID;
    std::string name;
    std::vector<SLParameter> parameters;
    SLBlockPtr body;
    unsigned int line = 0;
    unsigned int column = 0;
};

class SLShader final {
 public:
    /**
     * Whether this phase executes a shader of this type. A displacement or a volume shader
     * parses and is reported by name, which is the difference between a scene naming
     * something unsupported and a scene that is malformed.
     **/
    bool supported() const;

    SLShaderType type = SLShaderType::SURFACE;
    std::string name;
    std::vector<SLParameter> parameters;
    std::vector<SLFunction> functions;
    SLBlockPtr body;
    unsigned int line = 0;
    unsigned int column = 0;
};

typedef boost::shared_ptr<SLShader> SLShaderPtr;

};  // namespace v3d::render::offline
