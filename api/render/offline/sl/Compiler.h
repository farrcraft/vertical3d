/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "Syntax.h"

namespace v3d::render::offline::sl {

/**
 * One named value a shader run holds.
 *
 * A local declared twice in nested scopes is two symbols, so this is what the machine
 * allocates a register against rather than the name. Order is globals, then the shader's
 * parameters, then every local and formal in the order they were declared.
 **/
class Symbol final {
 public:
    enum class Role {
        /** A shader global: P, N, Ci and the rest, decided by the shader type. **/
        GLOBAL,
        /** A shader parameter, which a scene may bind. **/
        PARAMETER,
        /** A local, or the formal of a function defined inside the shader. **/
        LOCAL
    };

    std::string name;
    Type type = Type::FLOAT;
    Storage storage = Storage::UNIFORM;
    Role role = Role::LOCAL;
    /** Whether a shader of this type may assign it. **/
    bool writable = true;
    /** Whether the storage was written down rather than inferred. **/
    bool declared = false;
    /** Whether a shader parameter is written back to the caller. **/
    bool output = false;
};

/**
 * Symbols, types and the varying inference: the pass between the syntax tree and the
 * program.
 *
 * The tree is annotated in place - every expression comes out with a type and a storage
 * class, and every variable with the index of the symbol it resolved to.
 *
 * **The varying inference is the part that can be wrong quietly.** A value is uniform until
 * something varying reaches it; inferring uniform where varying was right gives a whole grid
 * one point's answer, which reads as a shading bug and is a compiler bug. Two things make it
 * sound rather than merely plausible: an assignment inside control flow whose condition is
 * varying becomes varying, because different points take different arms; and the walk runs
 * to a fixed point, because a loop can carry a varying value back to a name that was read
 * before it was written.
 **/
class Compiler final {
 public:
    explicit Compiler(const ShaderPtr & shader);

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
    void checkBlock(const BlockPtr & block);
    void checkStatement(const StatementPtr & statement);
    void checkCondition(const ExpressionPtr & condition, const char* construct);
    void checkJump(const StatementPtr & statement);
    void checkDeclaration(const StatementPtr & statement);
    void checkAssignment(const StatementPtr & statement);
    void checkLighting(const StatementPtr & statement);
    Type checkExpression(const ExpressionPtr & expression);
    Type checkVariable(const ExpressionPtr & expression);
    Type checkCall(const ExpressionPtr & expression);
    /**
     * Which of the shader's own functions the call names, or -1 for none of them. A shader's
     * function wins over a standard one of the same name.
     **/
    int checkShaderCall(Call & call, const std::vector<Type> & given);
    Type checkBuiltinCall(Call & call, const std::vector<Type> & given);
    Type checkUnary(const ExpressionPtr & expression);
    Type checkBinary(const ExpressionPtr & expression);
    Type checkTernary(const ExpressionPtr & expression);
    Type checkCast(const ExpressionPtr & expression);

    /**
     * The storage pass, run over and over until nothing changes. Marking a symbol varying is
     * the only direction anything moves, so it terminates.
     **/
    void infer();
    void inferBlock(const BlockPtr & block, bool varyingContext);
    void inferStatement(const StatementPtr & statement, bool varyingContext);
    void inferDeclaration(const StatementPtr & statement, bool varyingContext);
    void inferAssignment(const StatementPtr & statement, bool varyingContext);
    void inferJump(const StatementPtr & statement, bool varyingContext);
    /** Whether a break or a continue leaves this loop under a varying condition. **/
    bool escapes(const StatementPtr & loop) const;
    void mark(const Statement* loop);
    Storage inferExpression(const ExpressionPtr & expression);
    Storage inferCall(const ExpressionPtr & expression);
    /**
     * Mark a symbol varying, recording that something moved so the fixed point runs again.
     * A symbol that was declared uniform is left alone and the shader is faulted instead.
     **/
    void spread(int symbol, const ExpressionPtr & from);

    Failure fail(const std::string & message, unsigned int line, unsigned int column);

    ShaderPtr shader_;
    std::vector<Symbol> symbols_;
    std::vector<Binding> scope_;
    /**
     * The storage each function's result came out as, one per shader function, joined over
     * its return statements.
     **/
    std::vector<Storage> results_;
    /**
     * Which functions each function calls, for the recursion check.
     **/
    std::vector<std::vector<int> > calls_;
    /**
     * The loops being walked, innermost last, so that a break or a continue can name the one
     * it leaves.
     **/
    std::vector<const Statement*> enclosing_;
    /**
     * The loops a break or a continue escapes under a varying condition. Everything in such a
     * loop's body is varying whatever reached it, because the statements after the escape run
     * for some lanes and not for others.
     **/
    std::vector<const Statement*> escaping_;
    /**
     * The globals a surface shader may read only inside an illuminance body - L and Cl,
     * which a light sets and which mean nothing outside one.
     **/
    std::vector<int> lighting_;
    std::string error_;
    /**
     * A uniform that a varying value reached, held until the fixed point has settled: the
     * inference cannot report while it is still running, because a symbol may become varying
     * on a later round than the one that read it.
     **/
    std::string violation_;
    bool changed_ = false;
    /** Which function is being walked, or -1 for the shader body. **/
    int inside_ = -1;
    /** How many lighting constructs enclose the statement being checked. **/
    int depth_ = 0;
};

};  // namespace v3d::render::offline::sl
