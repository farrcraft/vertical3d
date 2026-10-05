/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Inference.h"

#include <api/render/offline/sl/syntax/Assignment.h>
#include <api/render/offline/sl/syntax/Call.h>
#include <api/render/offline/sl/syntax/Conditional.h>
#include <api/render/offline/sl/syntax/Declaration.h>
#include <api/render/offline/sl/syntax/Declarator.h>
#include <api/render/offline/sl/syntax/ExpressionStatement.h>
#include <api/render/offline/sl/syntax/For.h>
#include <api/render/offline/sl/syntax/Function.h>
#include <api/render/offline/sl/syntax/Jump.h>
#include <api/render/offline/sl/syntax/Lighting.h>
#include <api/render/offline/sl/syntax/Variable.h>
#include <api/render/offline/sl/syntax/Walk.h>
#include <api/render/offline/sl/syntax/While.h>

#include <algorithm>
#include <string>
#include <vector>

#include "Builtins.h"

namespace v3d::render::offline::sl {

namespace {

Storage join(Storage left, Storage right) {
    return left == Storage::VARYING || right == Storage::VARYING ?
        Storage::VARYING : Storage::UNIFORM;
}

};  // namespace

Inference::Inference(const syntax::ShaderPtr & shader, std::vector<Symbol>* symbols) :
    shader_(shader), symbols_(symbols) {
}

bool Inference::escapes(const syntax::StatementPtr & loop) const {
    return std::ranges::find(escaping_, loop.get()) != escaping_.end();
}

void Inference::mark(const syntax::Statement* loop) {
    if (std::ranges::find(escaping_, loop) != escaping_.end()) {
        return;
    }
    escaping_.push_back(loop);
    changed_ = true;
}

void Inference::spread(int symbol, const syntax::ExpressionPtr & from) {
    Symbol & entry = (*symbols_)[static_cast<std::size_t>(symbol)];
    if (entry.storage == Storage::VARYING) {
        return;
    }
    if (entry.declared) {
        // an explicit uniform that a varying value reaches is an error; silently keeping it
        // uniform would give a whole grid one point's value
        if (violation_.empty()) {
            const unsigned int line = from ? from->line : shader_->line;
            const unsigned int column = from ? from->column : shader_->column;
            violation_ = "'" + entry.name + "' is uniform and is given a varying value at line " +
                std::to_string(line) + ", column " + std::to_string(column);
        }
        return;
    }
    entry.storage = Storage::VARYING;
    changed_ = true;
}

std::string Inference::run() {
    // a loop can carry a varying value back to a name that was read before it was written,
    // so one pass is not enough. Nothing ever changes from varying back to uniform, so the
    // traversal is monotone and terminates
    results_.assign(shader_->functions.size(), Storage::UNIFORM);
    for (int round = 0; round < 64; round++) {
        changed_ = false;
        for (std::size_t i = 0; i < shader_->functions.size(); i++) {
            inside_ = static_cast<int>(i);
            inferBlock(shader_->functions[i].body, false);
        }
        inside_ = -1;
        inferBlock(shader_->body, false);
        if (!changed_) {
            break;
        }
    }
    return violation_;
}

void Inference::inferBlock(const syntax::BlockPtr & block, bool varyingContext) {
    if (!block) {
        return;
    }
    for (const syntax::StatementPtr & statement : block->statements) {
        inferStatement(statement, varyingContext);
    }
}

void Inference::inferStatement(const syntax::StatementPtr & statement, bool varyingContext) {
    if (!statement) {
        return;
    }
    switch (statement->kind) {
        case syntax::Statement::Kind::BLOCK:
            inferBlock(boost::static_pointer_cast<syntax::Block>(statement), varyingContext);
            return;
        case syntax::Statement::Kind::DECLARATION:
            inferDeclaration(statement, varyingContext);
            return;
        case syntax::Statement::Kind::ASSIGNMENT:
            inferAssignment(statement, varyingContext);
            return;
        case syntax::Statement::Kind::CONDITIONAL: {
            const syntax::Conditional & conditional = static_cast<const syntax::Conditional &>(*statement);
            const bool varying = inferExpression(conditional.condition) == Storage::VARYING;
            inferStatement(conditional.whenTrue, varyingContext || varying);
            inferStatement(conditional.whenFalse, varyingContext || varying);
            return;
        }
        case syntax::Statement::Kind::WHILE: {
            const syntax::While & loop = static_cast<const syntax::While &>(*statement);
            const bool varying = inferExpression(loop.condition) == Storage::VARYING;
            enclosing_.push_back(statement.get());
            inferStatement(loop.body, varyingContext || varying || escapes(statement));
            enclosing_.pop_back();
            return;
        }
        case syntax::Statement::Kind::FOR: {
            const syntax::For & loop = static_cast<const syntax::For &>(*statement);
            inferStatement(loop.initialiser, varyingContext);
            const bool varying = loop.condition ?
                inferExpression(loop.condition) == Storage::VARYING : false;
            const bool inside = varyingContext || varying || escapes(statement);
            enclosing_.push_back(statement.get());
            inferStatement(loop.step, inside);
            inferStatement(loop.body, inside);
            enclosing_.pop_back();
            return;
        }
        case syntax::Statement::Kind::JUMP:
            inferJump(statement, varyingContext);
            return;
        case syntax::Statement::Kind::EXPRESSION: {
            const syntax::ExpressionPtr & expression = static_cast<const syntax::ExpressionStatement &>(*statement).expression;
            inferExpression(expression);
            inferOutputs(expression, varyingContext);
            return;
        }
        case syntax::Statement::Kind::LIGHTING: {
            const syntax::Lighting & lighting = static_cast<const syntax::Lighting &>(*statement);
            for (const syntax::ExpressionPtr & argument : lighting.arguments) {
                inferExpression(argument);
            }
            // the body runs once per light with L and Cl set per point, so everything it
            // writes is varying whatever reached it
            inferStatement(lighting.body, true);
            return;
        }
    }
}

void Inference::inferDeclaration(const syntax::StatementPtr & statement, bool varyingContext) {
    const syntax::Declaration & declaration = static_cast<const syntax::Declaration &>(*statement);
    for (const syntax::Declarator & declarator : declaration.declarators) {
        const Storage value = declarator.initialiser ?
            inferExpression(declarator.initialiser) : Storage::UNIFORM;
        if (varyingContext || value == Storage::VARYING) {
            spread(declarator.symbol, declarator.initialiser);
        }
    }
}

void Inference::inferAssignment(const syntax::StatementPtr & statement, bool varyingContext) {
    const syntax::Assignment & assignment = static_cast<const syntax::Assignment &>(*statement);
    const Storage value = inferExpression(assignment.value);
    inferExpression(assignment.target);
    const syntax::Variable & variable = static_cast<const syntax::Variable &>(*assignment.target);
    // different points take different arms, so anything written under a varying condition is
    // varying whatever was written to it
    if (varyingContext || value == Storage::VARYING) {
        spread(variable.symbol, assignment.value);
    }
}

void Inference::inferOutputs(const syntax::ExpressionPtr & expression, bool varyingContext) {
    // a function that writes its arguments returns nothing, so it is only ever a statement
    if (!expression || expression->kind != syntax::Expression::Kind::CALL) {
        return;
    }
    const syntax::Call & call = static_cast<const syntax::Call &>(*expression);
    if (call.function >= 0 || call.signature < 0) {
        return;
    }
    const Signature & signature = builtins()[static_cast<std::size_t>(call.signature)];
    if (signature.outputs < 0) {
        return;
    }
    // what is written is as varying as what was read, which is an assignment's rule
    bool varying = varyingContext || signature.varying;
    const std::size_t first = static_cast<std::size_t>(signature.outputs);
    for (std::size_t argument = 0; argument < first && argument < call.arguments.size(); argument++) {
        varying = varying || call.arguments[argument]->storage == Storage::VARYING;
    }
    if (!varying) {
        return;
    }
    for (std::size_t argument = first; argument < call.arguments.size(); argument++) {
        spread(static_cast<const syntax::Variable &>(*call.arguments[argument]).symbol, expression);
    }
}

void Inference::inferJump(const syntax::StatementPtr & statement, bool varyingContext) {
    const syntax::Jump & jump = static_cast<const syntax::Jump &>(*statement);
    if (jump.where != syntax::Jump::Where::RETURN) {
        // a lane that breaks or continues under a varying condition leaves the ones beside it
        // running, so what the rest of the loop body writes differs per point
        if (varyingContext && !enclosing_.empty()) {
            mark(enclosing_.back());
        }
        return;
    }
    if (!jump.value || inside_ < 0) {
        return;
    }
    const Storage value = inferExpression(jump.value);
    Storage & result = results_[static_cast<std::size_t>(inside_)];
    if (result != Storage::VARYING && (varyingContext || value == Storage::VARYING)) {
        result = Storage::VARYING;
        changed_ = true;
    }
}

Storage Inference::inferExpression(const syntax::ExpressionPtr & expression) {
    if (!expression) {
        return Storage::UNIFORM;
    }
    Storage storage = Storage::UNIFORM;
    switch (expression->kind) {
        case syntax::Expression::Kind::NUMBER:
        case syntax::Expression::Kind::STRING:
            // a string names a coordinate space, a texture or a message, and there is no
            // per-point one to name: allowing one would make every transform a runtime
            // string lookup
            storage = Storage::UNIFORM;
            break;
        case syntax::Expression::Kind::VARIABLE:
            storage = (*symbols_)[static_cast<std::size_t>(
                static_cast<const syntax::Variable &>(*expression).symbol)].storage;
            break;
        case syntax::Expression::Kind::UNARY:
        case syntax::Expression::Kind::BINARY:
        case syntax::Expression::Kind::TERNARY:
        case syntax::Expression::Kind::CAST:
        case syntax::Expression::Kind::TUPLE:
            // as varying as any operand - a cast's space is a name rather than an operand,
            // and a name is uniform
            syntax::forEachChild(*expression, [this, &storage](const syntax::ExpressionPtr & operand) {
                storage = join(storage, inferExpression(operand));
            });
            break;
        case syntax::Expression::Kind::CALL:
            storage = inferCall(expression);
            break;
        case syntax::Expression::Kind::INDEX:
            break;
    }
    expression->storage = storage;
    return storage;
}

Storage Inference::inferCall(const syntax::ExpressionPtr & expression) {
    const syntax::Call & call = static_cast<const syntax::Call &>(*expression);
    Storage storage = Storage::UNIFORM;
    std::vector<Storage> arguments;
    arguments.reserve(call.arguments.size());
    for (const syntax::ExpressionPtr & argument : call.arguments) {
        arguments.push_back(inferExpression(argument));
        storage = join(storage, arguments.back());
    }
    if (call.function < 0) {
        // a built-in that reads the shading point is varying however uniform its arguments
        // are: ambient() takes none and returns a different value at every point on a grid
        const bool varying = call.signature >= 0 &&
            builtins()[static_cast<std::size_t>(call.signature)].varying;
        return varying ? Storage::VARYING : storage;
    }
    const std::size_t index = static_cast<std::size_t>(call.function);
    const syntax::Function & function = shader_->functions[index];
    // a formal takes the storage of every argument any call site passes it, so a function
    // called once with a varying value is varying everywhere
    for (std::size_t i = 0; i < arguments.size() && i < function.parameters.size(); i++) {
        if (arguments[i] == Storage::VARYING) {
            spread(function.parameters[i].symbol, call.arguments[i]);
        }
    }
    return join(storage, results_[index]);
}

};  // namespace v3d::render::offline::sl
