/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/runtime/Instruction.h>
#include <api/render/offline/sl/runtime/Program.h>
#include <api/render/offline/sl/syntax/Block.h>
#include <api/render/offline/sl/syntax/Expression.h>
#include <api/render/offline/sl/syntax/Shader.h>
#include <api/render/offline/sl/syntax/Statement.h>

#include <string>
#include <vector>

#include "Symbol.h"
#include "Types.h"

namespace v3d::render::offline::sl {

/**
 * The annotated tree, turned into a flat program.
 *
 * Runs after Compiler, which is what put the type and the storage class on every
 * expression and the symbol index on every variable. The emitter reads those rather than
 * working any of it out again.
 *
 * **A uniform condition becomes a jump and a varying one becomes a mask.** That is the
 * decision this pass exists to make: a condition every point in the batch agrees about costs
 * a branch, and one they disagree about runs both arms with the lanes that took each.
 **/
class Emitter final {
 public:
    Emitter(const syntax::ShaderPtr & shader, const std::vector<Symbol> & symbols);

    /**
     * False when something in the shader has no instructions yet, which error() names.
     **/
    bool emit(runtime::Program* program);

    const std::string & error() const;

 private:
    class Failure final {};

    /** A register for a value of that type and storage, with no name. **/
    int temporary(Type type, Storage storage);
    int number(float value);
    int string(const std::string & text);
    /** Where the next instruction will go, for a jump that is patched afterwards. **/
    int here() const;
    int put(runtime::Opcode opcode, int target, int left, int right, const syntax::ExpressionPtr & where);
    void patch(int instruction, int target);

    void emitBlock(const syntax::BlockPtr & block);
    void emitStatement(const syntax::StatementPtr & statement);
    void emitDeclaration(const syntax::StatementPtr & statement);
    void emitAssignment(const syntax::StatementPtr & statement);
    void emitConditional(const syntax::StatementPtr & statement);
    void emitWhile(const syntax::StatementPtr & statement);
    void emitFor(const syntax::StatementPtr & statement);
    void emitLoop(const syntax::ExpressionPtr & condition, const syntax::StatementPtr & body,
        const syntax::StatementPtr & step);
    void emitJump(const syntax::StatementPtr & statement);
    /**
     * The three message passing constructs. Each is a loop or a mask over registers the
     * shader's own globals already are, which is why they are instructions rather than
     * calls into the library.
     **/
    void emitLighting(const syntax::StatementPtr & statement);
    /** The register a shader global is, which the lighting constructs read and write. **/
    int global(const char* name) const;

    int emitExpression(const syntax::ExpressionPtr & expression);
    int emitBinary(const syntax::ExpressionPtr & expression);
    int emitCall(const syntax::ExpressionPtr & expression);
    /**
     * A shader's own function, pasted in where it was called. A run has no call stack, so
     * there is nowhere for a call to return to; the recursion the compiler rejects at the
     * call graph is what makes pasting terminate.
     **/
    int emitInline(const syntax::ExpressionPtr & expression);
    int emitCast(const syntax::ExpressionPtr & expression);

    Failure fail(const std::string & message, unsigned int line, unsigned int column);

    syntax::ShaderPtr shader_;
    const std::vector<Symbol> & symbols_;
    runtime::Program* program_ = nullptr;
    /**
     * The register each inlined body in progress returns through, innermost last. Empty
     * while the shader body itself is being emitted, where a return returns nothing.
     **/
    std::vector<int> returns_;
    std::string error_;
};

};  // namespace v3d::render::offline::sl
