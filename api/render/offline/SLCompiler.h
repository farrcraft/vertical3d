/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "SLSyntax.h"

namespace v3d::render::offline {

/**
 * One named value a shader run holds.
 *
 * A local declared twice in nested scopes is two symbols, so this is what the machine
 * allocates a register against rather than the name. Order is globals, then the shader's
 * parameters, then every local and formal in the order they were declared.
 **/
class SLSymbol final {
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
    SLType type = SLType::FLOAT;
    SLStorage storage = SLStorage::UNIFORM;
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
class SLCompiler final {
 public:
    explicit SLCompiler(const SLShaderPtr & shader);

    /**
     * Check and annotate. False on the first error, which error() then names with a
     * position; the tree is left partly annotated and is not worth running.
     **/
    bool compile();

    const std::string & error() const;

    /**
     * Every symbol the shader holds, in the order the machine should allocate them.
     **/
    const std::vector<SLSymbol> & symbols() const;

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
    int declare(const std::string & name, SLType type, SLStorage storage, SLSymbol::Role role,
        bool writable, unsigned int line, unsigned int column);
    int lookup(const std::string & name) const;

    void checkFunctions();
    /**
     * Reject a function that reaches itself. The machine has a register file per shader run
     * and no call stack, so a recursive shader has no meaning to give - which is why this is
     * the call graph's answer rather than the parser's.
     **/
    void checkCallGraph();
    void checkBlock(const SLBlockPtr & block);
    void checkStatement(const SLStatementPtr & statement);
    void checkCondition(const SLExpressionPtr & condition, const char* construct);
    void checkJump(const SLStatementPtr & statement);
    void checkDeclaration(const SLStatementPtr & statement);
    void checkAssignment(const SLStatementPtr & statement);
    void checkLighting(const SLStatementPtr & statement);
    SLType checkExpression(const SLExpressionPtr & expression);
    SLType checkVariable(const SLExpressionPtr & expression);
    SLType checkCall(const SLExpressionPtr & expression);
    /**
     * Which of the shader's own functions the call names, or -1 for none of them. A shader's
     * function wins over a standard one of the same name.
     **/
    int checkShaderCall(SLCall & call, const std::vector<SLType> & given);
    SLType checkBuiltinCall(SLCall & call, const std::vector<SLType> & given);
    SLType checkUnary(const SLExpressionPtr & expression);
    SLType checkBinary(const SLExpressionPtr & expression);
    SLType checkTernary(const SLExpressionPtr & expression);
    SLType checkCast(const SLExpressionPtr & expression);

    /**
     * The storage pass, run over and over until nothing changes. Marking a symbol varying is
     * the only direction anything moves, so it terminates.
     **/
    void infer();
    void inferBlock(const SLBlockPtr & block, bool varyingContext);
    void inferStatement(const SLStatementPtr & statement, bool varyingContext);
    void inferDeclaration(const SLStatementPtr & statement, bool varyingContext);
    void inferAssignment(const SLStatementPtr & statement, bool varyingContext);
    void inferJump(const SLStatementPtr & statement, bool varyingContext);
    /** Whether a break or a continue leaves this loop under a varying condition. **/
    bool escapes(const SLStatementPtr & loop) const;
    void mark(const SLStatement* loop);
    SLStorage inferExpression(const SLExpressionPtr & expression);
    SLStorage inferCall(const SLExpressionPtr & expression);
    /**
     * Mark a symbol varying, recording that something moved so the fixed point runs again.
     * A symbol that was declared uniform is left alone and the shader is faulted instead.
     **/
    void spread(int symbol, const SLExpressionPtr & from);

    Failure fail(const std::string & message, unsigned int line, unsigned int column);

    SLShaderPtr shader_;
    std::vector<SLSymbol> symbols_;
    std::vector<Binding> scope_;
    /**
     * The storage each function's result came out as, one per shader function, joined over
     * its return statements.
     **/
    std::vector<SLStorage> results_;
    /**
     * Which functions each function calls, for the recursion check.
     **/
    std::vector<std::vector<int> > calls_;
    /**
     * The loops being walked, innermost last, so that a break or a continue can name the one
     * it leaves.
     **/
    std::vector<const SLStatement*> enclosing_;
    /**
     * The loops a break or a continue escapes under a varying condition. Everything in such a
     * loop's body is varying whatever reached it, because the statements after the escape run
     * for some lanes and not for others.
     **/
    std::vector<const SLStatement*> escaping_;
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

};  // namespace v3d::render::offline
