/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/syntax/Block.h>
#include <api/render/offline/sl/syntax/Call.h>
#include <api/render/offline/sl/syntax/Cast.h>
#include <api/render/offline/sl/syntax/Expression.h>
#include <api/render/offline/sl/syntax/Shader.h>
#include <api/render/offline/sl/syntax/Statement.h>

#include <string>
#include <vector>

#include "Symbol.h"
#include "Types.h"

namespace v3d::render::offline::sl {

class Signature;

/**
 * Resolves symbols, checks types and infers storage classes, between parsing and emitting.
 *
 * The tree is annotated in place. Every expression gets a type and a storage class, and
 * every variable gets the index of the symbol it resolved to. The storage classes come from
 * Inference, which runs after the type checks.
 **/
class Compiler final {
 public:
    explicit Compiler(const syntax::ShaderPtr & shader);

    /**
     * Check and annotate. False on the first error, and error() then describes it with a
     * line and column. The tree is then partly annotated and must not be emitted.
     **/
    bool compile();

    const std::string & error() const;

    /**
     * Every symbol the shader holds, in the order the emitter allocates their registers.
     **/
    const std::vector<Symbol> & symbols() const;

 private:
    /**
     * Thrown by the checks and caught by compile(), so that a check is not a chain of error
     * returns.
     **/
    class Failure final {};

    /**
     * One name in scope, and the symbol it stands for. The scope is a stack of these. When
     * a block closes, the stack is cut back to its size when the block opened.
     **/
    class Binding final {
     public:
        std::string name;
        int symbol = 0;
    };

    static constexpr unsigned int SURFACE = 1U << 0;
    static constexpr unsigned int LIGHT = 1U << 1;
    static constexpr unsigned int IMAGER = 1U << 2;
    static constexpr unsigned int DISPLACEMENT = 1U << 3;

    /**
     * A shader global: which shader types have it, and which of them may write it.
     *
     * The table follows the standard. A name in the table is reported as belonging to the
     * shader types that have it. A light shader that mentions `Ci` is told that `Ci` belongs to
     * a surface and an imager shader, not that it is undeclared.
     **/
    class Global final {
     public:
        const char* name;
        Type type;
        Storage storage;
        unsigned int shaders;
        unsigned int writable;
        /**
         * Readable only inside a lighting construct, in a shader that cannot write it. In a
         * surface shader, L and Cl are set by the light the illuminance body is running for.
         **/
        bool lighting;
    };

    static constexpr Global GLOBALS[] = {
        // a displacement shader moves the surface, so it writes P and N, and reads where it is
        { "P", Type::POINT, Storage::VARYING, SURFACE | LIGHT | IMAGER | DISPLACEMENT, DISPLACEMENT, false },
        { "N", Type::NORMAL, Storage::VARYING, SURFACE | DISPLACEMENT, DISPLACEMENT, false },
        { "Ng", Type::NORMAL, Storage::VARYING, SURFACE | DISPLACEMENT, 0, false },
        { "I", Type::VECTOR, Storage::VARYING, SURFACE | DISPLACEMENT, 0, false },
        { "E", Type::POINT, Storage::UNIFORM, SURFACE | DISPLACEMENT, 0, false },
        { "Cs", Type::COLOR, Storage::VARYING, SURFACE, 0, false },
        { "Os", Type::COLOR, Storage::VARYING, SURFACE, 0, false },
        { "s", Type::FLOAT, Storage::VARYING, SURFACE | DISPLACEMENT, 0, false },
        { "t", Type::FLOAT, Storage::VARYING, SURFACE | DISPLACEMENT, 0, false },
        { "u", Type::FLOAT, Storage::VARYING, SURFACE | DISPLACEMENT, 0, false },
        { "v", Type::FLOAT, Storage::VARYING, SURFACE | DISPLACEMENT, 0, false },
        { "du", Type::FLOAT, Storage::VARYING, SURFACE | DISPLACEMENT, 0, false },
        { "dv", Type::FLOAT, Storage::VARYING, SURFACE | DISPLACEMENT, 0, false },
        { "Ci", Type::COLOR, Storage::VARYING, SURFACE | IMAGER, SURFACE | IMAGER, false },
        { "Oi", Type::COLOR, Storage::VARYING, SURFACE | IMAGER, SURFACE | IMAGER, false },
        { "Ps", Type::POINT, Storage::VARYING, LIGHT, 0, false },
        // an imager reads and writes alpha. A pixel it paints counts as covered, and the
        // "background" imager writes alpha to record that
        { "alpha", Type::FLOAT, Storage::VARYING, IMAGER, IMAGER, false },
        // a light writes these. A surface reads them, and only inside an illuminance body
        { "L", Type::VECTOR, Storage::VARYING, SURFACE | LIGHT, LIGHT, true },
        { "Cl", Type::COLOR, Storage::VARYING, SURFACE | LIGHT, LIGHT, true }
    };

    static unsigned int bit(ShaderType type);

    /**
     * Which shader types a global belongs to, as prose for a diagnostic.
     **/
    static std::string owners(unsigned int shaders);

    static std::string article(ShaderType type);

    /**
     * The type a call's arguments promote to, as an arithmetic operator promotes its operands.
     * Void when two of them have no arithmetic between them, such as a colour and a point.
     **/
    static Type promoted(const std::vector<Type> & given);

    /**
     * Whether one way of calling a standard library function takes these argument types.
     **/
    static bool suits(const Signature & signature, const std::vector<Type> & given);

    static bool defines(const std::vector<syntax::Function> & functions, const std::string & name);

    /**
     * Every name an expression calls, added once. adopt() collects these from the tree before
     * any name has been resolved.
     **/
    static void gather(const syntax::ExpressionPtr & expression, std::vector<std::string>* called);

    static void gather(const syntax::StatementPtr & statement, std::vector<std::string>* called);

    void declareGlobals();
    void declareParameters();
    int declare(const std::string & name, Type type, Storage storage, Symbol::Role role,
        bool writable, unsigned int line, unsigned int column);
    int lookup(const std::string & name) const;

    /**
     * Add the library functions the shader calls, and the ones those call in turn, to the
     * shader's own function list.
     *
     * `diffuse` and the other functions from sources() are written in the shading language.
     * After this, the checker, the inference and the inliner see only one kind of function.
     * A function the shader defines itself is not replaced.
     **/
    void adopt();

    void checkFunctions();
    /**
     * Reject a function that calls itself, directly or through others. Calls are inlined
     * and the machine has no call stack, so a recursive shader cannot run. The check needs
     * the whole call graph, so the parser cannot make it.
     **/
    void checkCallGraph();
    void checkBlock(const syntax::BlockPtr & block);
    void checkStatement(const syntax::StatementPtr & statement);
    void checkCondition(const syntax::ExpressionPtr & condition, const char* construct);
    void checkJump(const syntax::StatementPtr & statement);
    void checkDeclaration(const syntax::StatementPtr & statement);
    void checkAssignment(const syntax::StatementPtr & statement);
    void checkLighting(const syntax::StatementPtr & statement);
    Type checkExpression(const syntax::ExpressionPtr & expression);
    Type checkVariable(const syntax::ExpressionPtr & expression);
    Type checkCall(const syntax::ExpressionPtr & expression);
    /**
     * Which of the shader's own functions the call names, or -1 for none of them. A shader's
     * function takes precedence over a standard one of the same name.
     **/
    int checkShaderCall(syntax::Call & call, const std::vector<Type> & given);
    /**
     * The first standard signature that accepts the call, preferring one that returns the
     * type of a cast around the call.
     **/
    Type checkBuiltinCall(syntax::Call & call, const std::vector<Type> & given, Type wanted);
    /** Refuse a call that writes an argument which is not a variable that can be assigned. **/
    /**
     * An argument to a formal declared output is copied back when the function returns, so it
     * has to be a variable the caller may assign, of the formal's own type.
     **/
    void checkOutputArgument(const syntax::Call & call, std::size_t argument, Type formal, Type given);
    /**
     * Give a two-argument faceforward its reference, Ng, as RI defines it. A shader with no Ng
     * is refused, and a shader that declares its own faceforward is left alone.
     **/
    void referenceNg(syntax::Call* call);
    void checkWritten(const syntax::Call & call, const Signature & signature);
    Type checkUnary(const syntax::ExpressionPtr & expression);
    Type checkBinary(const syntax::ExpressionPtr & expression);
    Type checkTernary(const syntax::ExpressionPtr & expression);
    Type checkCast(const syntax::ExpressionPtr & expression);
    /**
     * Reject a cast whose type cannot take the space named in front of it. A point, vector
     * or normal takes a coordinate space, and a colour takes a colour space.
     **/
    void checkSpace(const syntax::Cast & cast);

    Failure fail(const std::string & message, unsigned int line, unsigned int column);

    syntax::ShaderPtr shader_;
    std::vector<Symbol> symbols_;
    std::vector<Binding> scope_;
    /**
     * Which functions each function calls, for the recursion check.
     **/
    std::vector<std::vector<int> > calls_;
    /**
     * The globals a surface shader may read only inside an illuminance body: L and Cl. A
     * light sets them, and they have no value outside that body.
     **/
    std::vector<int> lighting_;
    std::string error_;
    /** Which function is being checked, or -1 for the shader body. **/
    int inside_ = -1;
    /** How many lighting constructs enclose the statement being checked. **/
    int depth_ = 0;
    /**
     * The type a cast requires of the call that is its operand, or void. Read by that call
     * alone, and cleared before its arguments are checked.
     **/
    Type wanted_ = Type::VOID;
};

};  // namespace v3d::render::offline::sl
