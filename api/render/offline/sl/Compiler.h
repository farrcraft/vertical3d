/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/syntax/Block.h>
#include <api/render/offline/sl/syntax/Call.h>
#include <api/render/offline/sl/syntax/Expression.h>
#include <api/render/offline/sl/syntax/Shader.h>
#include <api/render/offline/sl/syntax/Statement.h>

#include <string>
#include <vector>

#include "Symbol.h"
#include "Types.h"

namespace v3d::render::offline::sl {

/**
 * Symbols, types and the varying inference: the pass between the syntax tree and the
 * program.
 *
 * The tree is annotated in place - every expression comes out with a type and a storage
 * class, and every variable with the index of the symbol it resolved to. The storage is the
 * Inference's, run once the checking is done.
 **/
class Compiler final {
 public:
    explicit Compiler(const syntax::ShaderPtr & shader);

    /**
     * Check and annotate. False on the first error, which error() then names with a
     * position; the tree is left partly annotated and is not worth running.
     **/
    bool compile();

    const std::string & error() const;

    /**
     * Every symbol the shader holds, in the order the machine should allocate them.
     **/
    const std::vector<Symbol> & symbols() const;

 private:
    /**
     * Thrown by the checks and caught by compile(), for the reason the parser's failure is.
     **/
    class Failure final {};

    /**
     * One name in scope, and the symbol it stands for. A scope is a run of these, unwound
     * to a mark when the block closes.
     **/
    class Binding final {
     public:
        std::string name;
        int symbol = 0;
    };

    void declareGlobals();
    void declareParameters();
    int declare(const std::string & name, Type type, Storage storage, Symbol::Role role,
        bool writable, unsigned int line, unsigned int column);
    int lookup(const std::string & name) const;

    /**
     * Take on the library functions the shader calls, and the ones those call in turn.
     *
     * `diffuse` and its siblings are written in the language, so a shader that names one
     * gets it added to its own function list before anything else runs. Everything
     * downstream - the checker, the inference and the inliner - then sees one kind of
     * function rather than two, and a shader's own definition of the name wins because it
     * is already there.
     **/
    void adopt();

    void checkFunctions();
    /**
     * Reject a function that reaches itself. The machine has a register file per shader run
     * and no call stack, so a recursive shader has no meaning to give - which is why this is
     * the call graph's answer rather than the parser's.
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
     * function wins over a standard one of the same name.
     **/
    int checkShaderCall(syntax::Call & call, const std::vector<Type> & given);
    /**
     * The first standard signature that accepts the call, preferring one that answers the
     * type a cast around the call wants.
     **/
    Type checkBuiltinCall(syntax::Call & call, const std::vector<Type> & given, Type wanted);
    Type checkUnary(const syntax::ExpressionPtr & expression);
    Type checkBinary(const syntax::ExpressionPtr & expression);
    Type checkTernary(const syntax::ExpressionPtr & expression);
    Type checkCast(const syntax::ExpressionPtr & expression);

    Failure fail(const std::string & message, unsigned int line, unsigned int column);

    syntax::ShaderPtr shader_;
    std::vector<Symbol> symbols_;
    std::vector<Binding> scope_;
    /**
     * Which functions each function calls, for the recursion check.
     **/
    std::vector<std::vector<int> > calls_;
    /**
     * The globals a surface shader may read only inside an illuminance body - L and Cl,
     * which a light sets and which mean nothing outside one.
     **/
    std::vector<int> lighting_;
    std::string error_;
    /** Which function is being walked, or -1 for the shader body. **/
    int inside_ = -1;
    /** How many lighting constructs enclose the statement being checked. **/
    int depth_ = 0;
    /**
     * The type a cast wants from the call that is its operand, or void. Read by that call
     * alone, and cleared before its arguments are checked.
     **/
    Type wanted_ = Type::VOID;
};

};  // namespace v3d::render::offline::sl
