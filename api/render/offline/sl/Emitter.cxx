/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Emitter.h"

#include <api/render/offline/sl/syntax/Assignment.h>
#include <api/render/offline/sl/syntax/Binary.h>
#include <api/render/offline/sl/syntax/Block.h>
#include <api/render/offline/sl/syntax/Call.h>
#include <api/render/offline/sl/syntax/Cast.h>
#include <api/render/offline/sl/syntax/Conditional.h>
#include <api/render/offline/sl/syntax/Declaration.h>
#include <api/render/offline/sl/syntax/Declarator.h>
#include <api/render/offline/sl/syntax/Expression.h>
#include <api/render/offline/sl/syntax/ExpressionStatement.h>
#include <api/render/offline/sl/syntax/For.h>
#include <api/render/offline/sl/syntax/Function.h>
#include <api/render/offline/sl/syntax/Jump.h>
#include <api/render/offline/sl/syntax/Lighting.h>
#include <api/render/offline/sl/syntax/Number.h>
#include <api/render/offline/sl/syntax/Parameter.h>
#include <api/render/offline/sl/syntax/Shader.h>
#include <api/render/offline/sl/syntax/Statement.h>
#include <api/render/offline/sl/syntax/String.h>
#include <api/render/offline/sl/syntax/Ternary.h>
#include <api/render/offline/sl/syntax/Tuple.h>
#include <api/render/offline/sl/syntax/Unary.h>
#include <api/render/offline/sl/syntax/Variable.h>
#include <api/render/offline/sl/syntax/While.h>

#include <string>
#include <vector>

#include "Types.h"

namespace v3d::render::offline::sl {

namespace {

runtime::Opcode binaryOpcode(const std::string & op) {
    if (op == "+") {
        return runtime::Opcode::ADD;
    }
    if (op == "-") {
        return runtime::Opcode::SUBTRACT;
    }
    if (op == "*") {
        return runtime::Opcode::MULTIPLY;
    }
    if (op == "/") {
        return runtime::Opcode::DIVIDE;
    }
    if (op == ".") {
        return runtime::Opcode::DOT;
    }
    if (op == "^") {
        return runtime::Opcode::CROSS;
    }
    if (op == "<") {
        return runtime::Opcode::LESS;
    }
    if (op == "<=") {
        return runtime::Opcode::LESS_EQUAL;
    }
    if (op == ">") {
        return runtime::Opcode::GREATER;
    }
    if (op == ">=") {
        return runtime::Opcode::GREATER_EQUAL;
    }
    if (op == "==") {
        return runtime::Opcode::EQUAL;
    }
    if (op == "!=") {
        return runtime::Opcode::NOT_EQUAL;
    }
    if (op == "&&") {
        return runtime::Opcode::AND;
    }
    return runtime::Opcode::OR;
}

};  // namespace

Emitter::Emitter(const syntax::ShaderPtr & shader, const std::vector<Symbol> & symbols) :
    shader_(shader),
    symbols_(symbols) {
}

const std::string & Emitter::error() const {
    return error_;
}

Emitter::Failure Emitter::fail(const std::string & message, unsigned int line, unsigned int column) {
    if (error_.empty()) {
        error_ = message + " at line " + std::to_string(line) + ", column " + std::to_string(column);
    }
    return Failure();
}

int Emitter::temporary(Type type, Storage storage) {
    runtime::Register reg;
    reg.type = type;
    reg.storage = storage == Storage::UNSPECIFIED ? Storage::UNIFORM : storage;
    program_->registers.push_back(reg);
    return static_cast<int>(program_->registers.size()) - 1;
}

int Emitter::number(float value) {
    runtime::Register reg;
    reg.type = Type::FLOAT;
    reg.storage = Storage::UNIFORM;
    reg.constant = true;
    reg.value.push_back(value);
    program_->registers.push_back(reg);
    return static_cast<int>(program_->registers.size()) - 1;
}

int Emitter::string(const std::string & text) {
    runtime::Register reg;
    reg.type = Type::STRING;
    reg.storage = Storage::UNIFORM;
    reg.constant = true;
    reg.text = text;
    program_->registers.push_back(reg);
    return static_cast<int>(program_->registers.size()) - 1;
}

int Emitter::here() const {
    return static_cast<int>(program_->instructions.size());
}

int Emitter::put(runtime::Opcode opcode, int target, int left, int right, const syntax::ExpressionPtr & where) {
    runtime::Instruction instruction;
    instruction.opcode = opcode;
    instruction.target = target;
    instruction.left = left;
    instruction.right = right;
    if (where) {
        instruction.line = where->line;
        instruction.column = where->column;
    }
    program_->instructions.push_back(instruction);
    return static_cast<int>(program_->instructions.size()) - 1;
}

void Emitter::patch(int instruction, int target) {
    program_->instructions[static_cast<std::size_t>(instruction)].target = target;
}

bool Emitter::emit(runtime::Program* program) {
    program_ = program;
    program_->type = shader_->type;
    program_->name = shader_->name;
    program_->registers.clear();
    program_->instructions.clear();
    // the symbols come first and in the compiler's order, so a renderer binds a parameter or
    // reads Ci by the index the compiler gave it rather than by looking a name up
    for (const Symbol & symbol : symbols_) {
        runtime::Register reg;
        reg.type = symbol.type;
        reg.storage = symbol.storage;
        reg.name = symbol.name;
        reg.parameter = symbol.role == Symbol::Role::PARAMETER;
        program_->registers.push_back(reg);
    }
    program_->symbols = symbols_.size();
    try {
        // the declared defaults come first and are their own run: a default the body
        // computed would overwrite whatever a scene bound, once per grid
        for (const syntax::Parameter & parameter : shader_->parameters) {
            if (!parameter.defaultValue || parameter.symbol < 0) {
                continue;
            }
            const int value = emitExpression(parameter.defaultValue);
            put(runtime::Opcode::MOVE, parameter.symbol, value, -1, parameter.defaultValue);
        }
        program_->prologue = program_->instructions.size();
        emitBlock(shader_->body);
    } catch (const Failure &) {
        program_->instructions.clear();
        program_->prologue = 0;
        return false;
    }
    return true;
}

void Emitter::emitBlock(const syntax::BlockPtr & block) {
    if (!block) {
        return;
    }
    for (const syntax::StatementPtr & statement : block->statements) {
        emitStatement(statement);
    }
}

void Emitter::emitStatement(const syntax::StatementPtr & statement) {
    if (!statement) {
        return;
    }
    switch (statement->kind) {
        case syntax::Statement::Kind::BLOCK:
            emitBlock(boost::static_pointer_cast<syntax::Block>(statement));
            return;
        case syntax::Statement::Kind::DECLARATION:
            emitDeclaration(statement);
            return;
        case syntax::Statement::Kind::ASSIGNMENT:
            emitAssignment(statement);
            return;
        case syntax::Statement::Kind::CONDITIONAL:
            emitConditional(statement);
            return;
        case syntax::Statement::Kind::WHILE:
            emitWhile(statement);
            return;
        case syntax::Statement::Kind::FOR:
            emitFor(statement);
            return;
        case syntax::Statement::Kind::JUMP:
            emitJump(statement);
            return;
        case syntax::Statement::Kind::EXPRESSION:
            emitExpression(static_cast<const syntax::ExpressionStatement &>(*statement).expression);
            return;
        case syntax::Statement::Kind::LIGHTING:
            emitLighting(statement);
            return;
    }
}

void Emitter::emitDeclaration(const syntax::StatementPtr & statement) {
    const syntax::Declaration & declaration = static_cast<const syntax::Declaration &>(*statement);
    for (const syntax::Declarator & declarator : declaration.declarators) {
        if (!declarator.initialiser) {
            continue;
        }
        const int value = emitExpression(declarator.initialiser);
        put(runtime::Opcode::MOVE, declarator.symbol, value, -1, declarator.initialiser);
    }
}

void Emitter::emitAssignment(const syntax::StatementPtr & statement) {
    const syntax::Assignment & assignment = static_cast<const syntax::Assignment &>(*statement);
    const syntax::Variable & variable = static_cast<const syntax::Variable &>(*assignment.target);
    const int value = emitExpression(assignment.value);
    if (assignment.op == "=") {
        put(runtime::Opcode::MOVE, variable.symbol, value, -1, assignment.value);
        return;
    }
    // a compound form is the operator and then the assignment, over a temporary of the
    // target's own type so that "Ci *= 0.5" stays a colour
    const std::string op = assignment.op.substr(0, 1);
    const int combined = temporary(assignment.target->type, assignment.target->storage);
    put(binaryOpcode(op), combined, variable.symbol, value, assignment.value);
    put(runtime::Opcode::MOVE, variable.symbol, combined, -1, assignment.value);
}

void Emitter::emitConditional(const syntax::StatementPtr & statement) {
    const syntax::Conditional & conditional = static_cast<const syntax::Conditional &>(*statement);
    const int condition = emitExpression(conditional.condition);

    if (conditional.condition->storage != Storage::VARYING) {
        // the condition is the same at every point, so the arm not taken costs a branch
        // rather than a pass over the batch with an empty mask
        const int skip = put(runtime::Opcode::JUMP_IF_ZERO, -1, condition, -1, conditional.condition);
        emitStatement(conditional.whenTrue);
        if (!conditional.whenFalse) {
            patch(skip, here());
            return;
        }
        const int over = put(runtime::Opcode::JUMP, -1, -1, -1, conditional.condition);
        patch(skip, here());
        emitStatement(conditional.whenFalse);
        patch(over, here());
        return;
    }

    // the condition varies between points, so both arms run, each under the lanes that took it.
    // The else arm's mask reads the condition again after the true arm has run. A bare variable
    // is its own register, and the true arm may assign it, so it is copied first.
    int tested = condition;
    if (conditional.whenFalse && conditional.condition->kind == syntax::Expression::Kind::VARIABLE) {
        tested = temporary(conditional.condition->type, conditional.condition->storage);
        put(runtime::Opcode::MOVE, tested, condition, -1, conditional.condition);
    }
    const int taken = put(runtime::Opcode::MASK, -1, tested, -1, conditional.condition);
    emitStatement(conditional.whenTrue);
    patch(taken, here());
    put(runtime::Opcode::POP_MASK, -1, -1, -1, conditional.condition);
    if (!conditional.whenFalse) {
        return;
    }
    const int other = put(runtime::Opcode::MASK_NOT, -1, tested, -1, conditional.condition);
    emitStatement(conditional.whenFalse);
    patch(other, here());
    put(runtime::Opcode::POP_MASK, -1, -1, -1, conditional.condition);
}

void Emitter::emitWhile(const syntax::StatementPtr & statement) {
    const syntax::While & loop = static_cast<const syntax::While &>(*statement);
    emitLoop(loop.condition, loop.body, syntax::StatementPtr());
}

void Emitter::emitFor(const syntax::StatementPtr & statement) {
    const syntax::For & loop = static_cast<const syntax::For &>(*statement);
    emitStatement(loop.initialiser);
    emitLoop(loop.condition, loop.body, loop.step);
}

void Emitter::emitLoop(const syntax::ExpressionPtr & condition, const syntax::StatementPtr & body,
    const syntax::StatementPtr & step) {
    /*
        A loop is masked whether its condition varies or not. A uniform condition narrows the
        loop's lanes all together, so it behaves as the jump it would have compiled to. With
        one form, break and continue have one meaning rather than two.
    */
    const int open = put(runtime::Opcode::LOOP, -1, -1, -1, condition);
    const int top = here();
    if (condition) {
        const int test = emitExpression(condition);
        put(runtime::Opcode::LOOP_TEST, -1, test, -1, condition);
    }
    emitStatement(body);
    if (step) {
        // a lane that took a continue still runs the step, as C's continue does; otherwise
        // its counter would stop advancing and the loop would never end
        put(runtime::Opcode::LOOP_RESTORE, -1, -1, -1, condition);
        emitStatement(step);
    }
    put(runtime::Opcode::LOOP_END, top, -1, -1, condition);
    patch(open, here());
    put(runtime::Opcode::POP_LOOP, -1, -1, -1, condition);
}

void Emitter::emitJump(const syntax::StatementPtr & statement) {
    const syntax::Jump & jump = static_cast<const syntax::Jump &>(*statement);
    switch (jump.where) {
        case syntax::Jump::Where::BREAK:
            put(runtime::Opcode::BREAK, -1, -1, -1, syntax::ExpressionPtr());
            return;
        case syntax::Jump::Where::CONTINUE:
            put(runtime::Opcode::CONTINUE, -1, -1, -1, syntax::ExpressionPtr());
            return;
        case syntax::Jump::Where::RETURN:
            if (jump.value && !returns_.empty()) {
                const int value = emitExpression(jump.value);
                put(runtime::Opcode::MOVE, returns_.back(), value, -1, jump.value);
            }
            put(runtime::Opcode::RETURN, -1, -1, -1, jump.value);
            return;
    }
}

int Emitter::emitExpression(const syntax::ExpressionPtr & expression) {
    switch (expression->kind) {
        case syntax::Expression::Kind::NUMBER:
            return number(static_cast<const syntax::Number &>(*expression).value);
        case syntax::Expression::Kind::STRING:
            return string(static_cast<const syntax::String &>(*expression).value);
        case syntax::Expression::Kind::VARIABLE:
            return static_cast<const syntax::Variable &>(*expression).symbol;
        case syntax::Expression::Kind::UNARY: {
            const syntax::Unary & unary = static_cast<const syntax::Unary &>(*expression);
            const int operand = emitExpression(unary.operand);
            const int result = temporary(expression->type, expression->storage);
            put(unary.op == "!" ? runtime::Opcode::NOT : runtime::Opcode::NEGATE, result, operand, -1, expression);
            return result;
        }
        case syntax::Expression::Kind::BINARY:
            return emitBinary(expression);
        case syntax::Expression::Kind::TERNARY: {
            // both arms are computed and one is chosen: the same work a mask would do, in
            // fewer instructions
            const syntax::Ternary & ternary = static_cast<const syntax::Ternary &>(*expression);
            const int condition = emitExpression(ternary.condition);
            const int result = temporary(expression->type, expression->storage);
            const int whenTrue = emitExpression(ternary.whenTrue);
            const int whenFalse = emitExpression(ternary.whenFalse);
            const int taken = put(runtime::Opcode::MASK, -1, condition, -1, expression);
            put(runtime::Opcode::MOVE, result, whenTrue, -1, expression);
            patch(taken, here());
            put(runtime::Opcode::POP_MASK, -1, -1, -1, expression);
            const int other = put(runtime::Opcode::MASK_NOT, -1, condition, -1, expression);
            put(runtime::Opcode::MOVE, result, whenFalse, -1, expression);
            patch(other, here());
            put(runtime::Opcode::POP_MASK, -1, -1, -1, expression);
            return result;
        }
        case syntax::Expression::Kind::CAST:
            return emitCast(expression);
        case syntax::Expression::Kind::CALL:
            return emitCall(expression);
        case syntax::Expression::Kind::TUPLE:
        case syntax::Expression::Kind::INDEX:
            break;
    }
    throw fail("this expression has no instructions yet", expression->line, expression->column);
}

int Emitter::emitBinary(const syntax::ExpressionPtr & expression) {
    const syntax::Binary & binary = static_cast<const syntax::Binary &>(*expression);
    const int left = emitExpression(binary.left);
    const int right = emitExpression(binary.right);
    const int result = temporary(expression->type, expression->storage);
    put(binaryOpcode(binary.op), result, left, right, expression);
    return result;
}

int Emitter::emitCast(const syntax::ExpressionPtr & expression) {
    const syntax::Cast & cast = static_cast<const syntax::Cast &>(*expression);
    const int result = temporary(expression->type, expression->storage);
    if (cast.operand->kind == syntax::Expression::Kind::TUPLE) {
        // a parenthesised list is a literal for the type in front of it, so each element is
        // moved into its own component of the result
        const syntax::Tuple & tuple = static_cast<const syntax::Tuple &>(*cast.operand);
        for (std::size_t i = 0; i < tuple.elements.size(); i++) {
            const int element = emitExpression(tuple.elements[i]);
            put(runtime::Opcode::MOVE_COMPONENT, result, element, static_cast<int>(i), expression);
        }
    } else {
        const int operand = emitExpression(cast.operand);
        put(runtime::Opcode::MOVE, result, operand, -1, expression);
    }
    if (cast.space.empty()) {
        return result;
    }
    // a space name makes a cast a transform, and the type decides which transform: a point
    // translates, a vector does not, a normal goes by the inverse transpose
    const int space = string(cast.space);
    const int moved = temporary(expression->type, expression->storage);
    put(runtime::Opcode::TRANSFORM, moved, result, space, expression);
    return moved;
}

int Emitter::emitCall(const syntax::ExpressionPtr & expression) {
    const syntax::Call & call = static_cast<const syntax::Call &>(*expression);
    if (call.function >= 0) {
        return emitInline(expression);
    }
    runtime::Instruction instruction;
    instruction.opcode = runtime::Opcode::CALL;
    instruction.target = temporary(expression->type, expression->storage);
    instruction.left = call.signature;
    instruction.line = expression->line;
    instruction.column = expression->column;
    instruction.arguments.reserve(call.arguments.size());
    for (const syntax::ExpressionPtr & argument : call.arguments) {
        instruction.arguments.push_back(emitExpression(argument));
    }
    program_->instructions.push_back(instruction);
    return instruction.target;
}

int Emitter::global(const char* name) const {
    for (std::size_t i = 0; i < symbols_.size(); i++) {
        if (symbols_[i].role == Symbol::Role::GLOBAL && symbols_[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void Emitter::emitLighting(const syntax::StatementPtr & statement) {
    const syntax::Lighting & lighting = static_cast<const syntax::Lighting &>(*statement);
    syntax::ExpressionPtr where;
    std::vector<int> given;
    given.reserve(lighting.arguments.size());
    for (const syntax::ExpressionPtr & argument : lighting.arguments) {
        given.push_back(emitExpression(argument));
        where = argument;
    }

    runtime::Instruction open;
    open.arguments = given;
    open.line = statement->line;
    open.column = statement->column;
    if (lighting.construct != syntax::Lighting::Construct::ILLUMINANCE) {
        /*
            A light shader's end of the message passing. L and Ps are its own globals: the
            construct writes the first for every point it lights and reads the second for
            the position of each of those points.
        */
        open.opcode = lighting.construct == syntax::Lighting::Construct::ILLUMINATE
            ? runtime::Opcode::ILLUMINATE : runtime::Opcode::SOLAR;
        open.left = global("L");
        open.right = global("Ps");
        program_->instructions.push_back(open);
        const int skip = here() - 1;
        emitStatement(lighting.body);
        put(runtime::Opcode::POP_MASK, -1, -1, -1, where);
        patch(skip, here());
        return;
    }

    /*
        A surface shader's end. The body is a loop over the lights rather than over a
        condition, so it is its own opcode rather than the LOOP the machine already has:
        what narrows the batch is which points a light reaches, and that arrives from the
        renderer one light at a time.
    */
    open.opcode = runtime::Opcode::ILLUMINANCE;
    open.left = global("L");
    open.right = global("Cl");
    program_->instructions.push_back(open);
    const int top = here();
    const int done = put(runtime::Opcode::ILLUMINANCE_NEXT, -1, -1, -1, where);
    emitStatement(lighting.body);
    put(runtime::Opcode::POP_MASK, -1, -1, -1, where);
    put(runtime::Opcode::JUMP, top, -1, -1, where);
    patch(done, here());
    put(runtime::Opcode::POP_ILLUMINANCE, -1, -1, -1, where);
}

int Emitter::emitInline(const syntax::ExpressionPtr & expression) {
    const syntax::Call & call = static_cast<const syntax::Call &>(*expression);
    const syntax::Function & function = shader_->functions[static_cast<std::size_t>(call.function)];
    // every argument is evaluated before any formal is written, so that an argument which
    // is itself a call cannot land on a formal this one has already filled
    std::vector<int> given;
    given.reserve(call.arguments.size());
    for (const syntax::ExpressionPtr & argument : call.arguments) {
        given.push_back(emitExpression(argument));
    }
    for (std::size_t i = 0; i < given.size() && i < function.parameters.size(); i++) {
        put(runtime::Opcode::MOVE, function.parameters[i].symbol, given[i], -1, call.arguments[i]);
    }
    const int result = temporary(expression->type, expression->storage);
    put(runtime::Opcode::ENTER, -1, -1, -1, expression);
    returns_.push_back(result);
    emitBlock(function.body);
    returns_.pop_back();
    put(runtime::Opcode::LEAVE, -1, -1, -1, expression);
    return result;
}

};  // namespace v3d::render::offline::sl
