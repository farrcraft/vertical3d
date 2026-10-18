/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

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
 * Decides which symbols, expressions and function results are varying. It runs over a tree
 * the checker has already annotated with types and symbols.
 *
 * **A mistake in this pass produces no error.** A value is uniform until something varying
 * reaches it. Inferring uniform where varying was right gives a whole grid one point's
 * value, which looks like a shading bug but is a compiler bug. Two rules make the inference
 * sound. First, an assignment inside control flow with a varying condition is varying,
 * because different points take different arms. Second, the traversal repeats until nothing
 * changes, because a loop can carry a varying value back to a name that was read before it
 * was written. Each round only changes things from uniform to varying, so the rounds end.
 **/
class Inference final {
 public:
    /**
     * @param symbols the checker's symbols, whose storage this decides
     **/
    Inference(const syntax::ShaderPtr & shader, std::vector<Symbol>* symbols);

    /**
     * Annotate every expression with its storage and settle every symbol's.
     * @return an error naming the first symbol declared uniform that a varying value
     *         reached, or empty when there was none
     **/
    std::string run();

 private:
    void inferBlock(const syntax::BlockPtr & block, bool varyingContext);
    void inferStatement(const syntax::StatementPtr & statement, bool varyingContext);
    void inferDeclaration(const syntax::StatementPtr & statement, bool varyingContext);
    void inferAssignment(const syntax::StatementPtr & statement, bool varyingContext);
    /** A call that writes its arguments, which is an assignment to each of them. **/
    void inferOutputs(const syntax::ExpressionPtr & expression, bool varyingContext);
    void inferJump(const syntax::StatementPtr & statement, bool varyingContext);
    /** Whether a break or a continue leaves this loop under a varying condition. **/
    bool escapes(const syntax::StatementPtr & loop) const;
    void mark(const syntax::Statement* loop);
    Storage inferExpression(const syntax::ExpressionPtr & expression);
    Storage inferCall(const syntax::ExpressionPtr & expression);
    /**
     * Mark a symbol varying, and record the change so that another round runs. A symbol
     * declared uniform keeps its storage, and the first such case becomes run()'s error.
     **/
    void spread(int symbol, const syntax::ExpressionPtr & from);

    syntax::ShaderPtr shader_;
    std::vector<Symbol>* symbols_;
    /**
     * The storage each function's result came out as, one per shader function, joined over
     * its return statements.
     **/
    std::vector<Storage> results_;
    /**
     * The loops being traversed, innermost last. A break or a continue leaves the last one.
     **/
    std::vector<const syntax::Statement*> enclosing_;
    /**
     * The loops a break or a continue escapes under a varying condition. Everything in such a
     * loop's body is varying whatever reached it, because the statements after the escape run
     * for some lanes and not for others.
     **/
    std::vector<const syntax::Statement*> escaping_;
    /**
     * The error for the first symbol declared uniform that a varying value reached. run()
     * returns it after the last round.
     **/
    std::string violation_;
    bool changed_ = false;
    /** Which function is being traversed, or -1 for the shader body. **/
    int inside_ = -1;
};

};  // namespace v3d::render::offline::sl
