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
 * Runs after Compiler, which puts the type and the storage class on every expression and
 * the symbol index on every variable. The emitter reads those rather than working them out
 * again.
 *
 * **A uniform condition becomes a jump and a varying one becomes a mask.** A condition with
 * the same value at every point in the batch costs a branch. A condition whose value differs
 * between points runs both arms, each with the lanes that took it.
 **/
class Emitter final {
 public:
    Emitter(const syntax::ShaderPtr & shader, const std::vector<Symbol> & symbols);

    /**
     * False when the shader uses something that has no instructions; error() names it.
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
     * The three message passing constructs. Each is a loop or a mask over the registers that
     * hold the shader's own globals, so they are instructions rather than calls into the
     * library.
     **/
    void emitLighting(const syntax::StatementPtr & statement);
    /** The register that holds a shader global, which the lighting constructs read and write. **/
    int global(const char* name) const;

    int emitExpression(const syntax::ExpressionPtr & expression);
    int emitBinary(const syntax::ExpressionPtr & expression);
    int emitCall(const syntax::ExpressionPtr & expression);
    /**
     * A shader's own function, pasted in where it was called. A run has no call stack, so a
     * call has nowhere to return to. Pasting terminates because the compiler rejects
     * recursion.
     **/
    int emitInline(const syntax::ExpressionPtr & expression);
    int emitCast(const syntax::ExpressionPtr & expression);
    /**
     * A colour whose values were given in a named colour space, converted into rgb as
     * ctransform converts it. The value's own register when the space is already rgb.
     **/
    int emitColourSpace(const std::string & space, int value, const syntax::ExpressionPtr & expression);

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
