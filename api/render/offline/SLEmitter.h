/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "SLCompiler.h"
#include "SLProgram.h"
#include "SLSyntax.h"

namespace v3d::render::offline {

/**
 * The annotated tree, turned into a flat program.
 *
 * Runs after SLCompiler, which is what put the type and the storage class on every
 * expression and the symbol index on every variable. The emitter reads those rather than
 * working any of it out again.
 *
 * **A uniform condition becomes a jump and a varying one becomes a mask.** That is the
 * decision this pass exists to make: a condition every point in the batch agrees about costs
 * a branch, and one they disagree about runs both arms with the lanes that took each.
 **/
class SLEmitter final {
 public:
    SLEmitter(const SLShaderPtr & shader, const std::vector<SLSymbol> & symbols);

    /**
     * False when something in the shader has no instructions yet, which error() names.
     **/
    bool emit(SLProgram* program);

    const std::string & error() const;

 private:
    class Failure final {};

    /** A register for a value of that type and storage, with no name. **/
    int temporary(SLType type, SLStorage storage);
    int number(float value);
    int string(const std::string & text);
    /** Where the next instruction will go, for a jump that is patched afterwards. **/
    int here() const;
    int put(SLOpcode opcode, int target, int left, int right, const SLExpressionPtr & where);
    void patch(int instruction, int target);

    void emitBlock(const SLBlockPtr & block);
    void emitStatement(const SLStatementPtr & statement);
    void emitDeclaration(const SLStatementPtr & statement);
    void emitAssignment(const SLStatementPtr & statement);
    void emitConditional(const SLStatementPtr & statement);
    void emitWhile(const SLStatementPtr & statement);
    void emitFor(const SLStatementPtr & statement);
    void emitLoop(const SLExpressionPtr & condition, const SLStatementPtr & body,
        const SLStatementPtr & step);
    void emitJump(const SLStatementPtr & statement);

    int emitExpression(const SLExpressionPtr & expression);
    int emitBinary(const SLExpressionPtr & expression);
    int emitCall(const SLExpressionPtr & expression);
    int emitCast(const SLExpressionPtr & expression);

    Failure fail(const std::string & message, unsigned int line, unsigned int column);

    SLShaderPtr shader_;
    const std::vector<SLSymbol> & symbols_;
    SLProgram* program_ = nullptr;
    std::string error_;
};

};  // namespace v3d::render::offline
