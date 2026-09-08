/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SLEmitter.h"

#include <string>
#include <vector>

#include "SLTypes.h"

namespace v3d::render::offline {

namespace {

SLOpcode binaryOpcode(const std::string & op) {
    if (op == "+") {
        return SLOpcode::ADD;
    }
    if (op == "-") {
        return SLOpcode::SUBTRACT;
    }
    if (op == "*") {
        return SLOpcode::MULTIPLY;
    }
    if (op == "/") {
        return SLOpcode::DIVIDE;
    }
    if (op == ".") {
        return SLOpcode::DOT;
    }
    if (op == "^") {
        return SLOpcode::CROSS;
    }
    if (op == "<") {
        return SLOpcode::LESS;
    }
    if (op == "<=") {
        return SLOpcode::LESS_EQUAL;
    }
    if (op == ">") {
        return SLOpcode::GREATER;
    }
    if (op == ">=") {
        return SLOpcode::GREATER_EQUAL;
    }
    if (op == "==") {
        return SLOpcode::EQUAL;
    }
    if (op == "!=") {
        return SLOpcode::NOT_EQUAL;
    }
    if (op == "&&") {
        return SLOpcode::AND;
    }
    return SLOpcode::OR;
}

};  // namespace

SLEmitter::SLEmitter(const SLShaderPtr & shader, const std::vector<SLSymbol> & symbols) :
    shader_(shader),
    symbols_(symbols) {
}

const std::string & SLEmitter::error() const {
    return error_;
}

SLEmitter::Failure SLEmitter::fail(const std::string & message, unsigned int line, unsigned int column) {
    if (error_.empty()) {
        error_ = message + " at line " + std::to_string(line) + ", column " + std::to_string(column);
    }
    return Failure();
}

int SLEmitter::temporary(SLType type, SLStorage storage) {
    SLRegister reg;
    reg.type = type;
    reg.storage = storage == SLStorage::UNSPECIFIED ? SLStorage::UNIFORM : storage;
    program_->registers.push_back(reg);
    return static_cast<int>(program_->registers.size()) - 1;
}

int SLEmitter::number(float value) {
    SLRegister reg;
    reg.type = SLType::FLOAT;
    reg.storage = SLStorage::UNIFORM;
    reg.constant = true;
    reg.value.push_back(value);
    program_->registers.push_back(reg);
    return static_cast<int>(program_->registers.size()) - 1;
}

int SLEmitter::string(const std::string & text) {
    SLRegister reg;
    reg.type = SLType::STRING;
    reg.storage = SLStorage::UNIFORM;
    reg.constant = true;
    reg.text = text;
    program_->registers.push_back(reg);
    return static_cast<int>(program_->registers.size()) - 1;
}

int SLEmitter::here() const {
    return static_cast<int>(program_->instructions.size());
}

int SLEmitter::put(SLOpcode opcode, int target, int left, int right, const SLExpressionPtr & where) {
    SLInstruction instruction;
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

void SLEmitter::patch(int instruction, int target) {
    program_->instructions[static_cast<std::size_t>(instruction)].target = target;
}

bool SLEmitter::emit(SLProgram* program) {
    program_ = program;
    program_->type = shader_->type;
    program_->name = shader_->name;
    program_->registers.clear();
    program_->instructions.clear();
    // the symbols come first and in the compiler's order, so a renderer binds a parameter or
    // reads Ci by the index the compiler gave it rather than by looking a name up
    for (const SLSymbol & symbol : symbols_) {
        SLRegister reg;
        reg.type = symbol.type;
        reg.storage = symbol.storage;
        reg.name = symbol.name;
        program_->registers.push_back(reg);
    }
    program_->symbols = symbols_.size();
    try {
        emitBlock(shader_->body);
    } catch (const Failure &) {
        program_->instructions.clear();
        return false;
    }
    return true;
}

void SLEmitter::emitBlock(const SLBlockPtr & block) {
    if (!block) {
        return;
    }
    for (const SLStatementPtr & statement : block->statements) {
        emitStatement(statement);
    }
}

void SLEmitter::emitStatement(const SLStatementPtr & statement) {
    if (!statement) {
        return;
    }
    switch (statement->kind) {
        case SLStatement::Kind::BLOCK:
            emitBlock(boost::static_pointer_cast<SLBlock>(statement));
            return;
        case SLStatement::Kind::DECLARATION:
            emitDeclaration(statement);
            return;
        case SLStatement::Kind::ASSIGNMENT:
            emitAssignment(statement);
            return;
        case SLStatement::Kind::CONDITIONAL:
            emitConditional(statement);
            return;
        case SLStatement::Kind::WHILE:
            emitWhile(statement);
            return;
        case SLStatement::Kind::FOR:
            emitFor(statement);
            return;
        case SLStatement::Kind::JUMP:
            emitJump(statement);
            return;
        case SLStatement::Kind::EXPRESSION:
            emitExpression(static_cast<const SLExpressionStatement &>(*statement).expression);
            return;
        case SLStatement::Kind::LIGHTING:
            // illuminance runs another shader's program over the same batch, which is the
            // standard library's message passing rather than an instruction
            throw fail("a lighting construct has no instructions yet",
                statement->line, statement->column);
    }
}

void SLEmitter::emitDeclaration(const SLStatementPtr & statement) {
    const SLDeclaration & declaration = static_cast<const SLDeclaration &>(*statement);
    for (const SLDeclarator & declarator : declaration.declarators) {
        if (!declarator.initialiser) {
            continue;
        }
        const int value = emitExpression(declarator.initialiser);
        put(SLOpcode::MOVE, declarator.symbol, value, -1, declarator.initialiser);
    }
}

void SLEmitter::emitAssignment(const SLStatementPtr & statement) {
    const SLAssignment & assignment = static_cast<const SLAssignment &>(*statement);
    const SLVariable & variable = static_cast<const SLVariable &>(*assignment.target);
    const int value = emitExpression(assignment.value);
    if (assignment.op == "=") {
        put(SLOpcode::MOVE, variable.symbol, value, -1, assignment.value);
        return;
    }
    // a compound form is the operator and then the assignment, over a temporary of the
    // target's own type so that "Ci *= 0.5" stays a colour
    const std::string op = assignment.op.substr(0, 1);
    const int combined = temporary(assignment.target->type, assignment.target->storage);
    put(binaryOpcode(op), combined, variable.symbol, value, assignment.value);
    put(SLOpcode::MOVE, variable.symbol, combined, -1, assignment.value);
}

void SLEmitter::emitConditional(const SLStatementPtr & statement) {
    const SLConditional & conditional = static_cast<const SLConditional &>(*statement);
    const int condition = emitExpression(conditional.condition);

    if (conditional.condition->storage != SLStorage::VARYING) {
        // every point agrees, so the arm not taken costs a branch rather than a pass over
        // the batch with an empty mask
        const int skip = put(SLOpcode::JUMP_IF_ZERO, -1, condition, -1, conditional.condition);
        emitStatement(conditional.whenTrue);
        if (!conditional.whenFalse) {
            patch(skip, here());
            return;
        }
        const int over = put(SLOpcode::JUMP, -1, -1, -1, conditional.condition);
        patch(skip, here());
        emitStatement(conditional.whenFalse);
        patch(over, here());
        return;
    }

    // the points disagree, so both arms run, each under the lanes that took it
    const int taken = put(SLOpcode::MASK, -1, condition, -1, conditional.condition);
    emitStatement(conditional.whenTrue);
    patch(taken, here());
    put(SLOpcode::POP_MASK, -1, -1, -1, conditional.condition);
    if (!conditional.whenFalse) {
        return;
    }
    const int other = put(SLOpcode::MASK_NOT, -1, condition, -1, conditional.condition);
    emitStatement(conditional.whenFalse);
    patch(other, here());
    put(SLOpcode::POP_MASK, -1, -1, -1, conditional.condition);
}

void SLEmitter::emitWhile(const SLStatementPtr & statement) {
    const SLWhile & loop = static_cast<const SLWhile &>(*statement);
    emitLoop(loop.condition, loop.body, SLStatementPtr());
}

void SLEmitter::emitFor(const SLStatementPtr & statement) {
    const SLFor & loop = static_cast<const SLFor &>(*statement);
    emitStatement(loop.initialiser);
    emitLoop(loop.condition, loop.body, loop.step);
}

void SLEmitter::emitLoop(const SLExpressionPtr & condition, const SLStatementPtr & body,
    const SLStatementPtr & step) {
    /*
        A loop is masked whether its condition varies or not. A uniform condition narrows the
        loop's lanes all together, so it behaves as the jump it would have compiled to, and
        one form means break and continue have one meaning rather than two.
    */
    const int open = put(SLOpcode::LOOP, -1, -1, -1, condition);
    const int top = here();
    if (condition) {
        const int test = emitExpression(condition);
        put(SLOpcode::LOOP_TEST, -1, test, -1, condition);
    }
    emitStatement(body);
    if (step) {
        // a lane that took a continue comes back for the step: C's continue goes to it
        // rather than past it, and a counter that stopped advancing would never end
        put(SLOpcode::LOOP_RESTORE, -1, -1, -1, condition);
        emitStatement(step);
    }
    put(SLOpcode::LOOP_END, top, -1, -1, condition);
    patch(open, here());
    put(SLOpcode::POP_LOOP, -1, -1, -1, condition);
}

void SLEmitter::emitJump(const SLStatementPtr & statement) {
    const SLJump & jump = static_cast<const SLJump &>(*statement);
    switch (jump.where) {
        case SLJump::Where::BREAK:
            put(SLOpcode::BREAK, -1, -1, -1, SLExpressionPtr());
            return;
        case SLJump::Where::CONTINUE:
            put(SLOpcode::CONTINUE, -1, -1, -1, SLExpressionPtr());
            return;
        case SLJump::Where::RETURN:
            put(SLOpcode::RETURN, -1, -1, -1, jump.value);
            return;
    }
}

int SLEmitter::emitExpression(const SLExpressionPtr & expression) {
    switch (expression->kind) {
        case SLExpression::Kind::NUMBER:
            return number(static_cast<const SLNumber &>(*expression).value);
        case SLExpression::Kind::STRING:
            return string(static_cast<const SLString &>(*expression).value);
        case SLExpression::Kind::VARIABLE:
            return static_cast<const SLVariable &>(*expression).symbol;
        case SLExpression::Kind::UNARY: {
            const SLUnary & unary = static_cast<const SLUnary &>(*expression);
            const int operand = emitExpression(unary.operand);
            const int result = temporary(expression->type, expression->storage);
            put(unary.op == "!" ? SLOpcode::NOT : SLOpcode::NEGATE, result, operand, -1, expression);
            return result;
        }
        case SLExpression::Kind::BINARY:
            return emitBinary(expression);
        case SLExpression::Kind::TERNARY: {
            // both arms are computed and one is chosen, which is what a mask would do with
            // fewer instructions and the same work
            const SLTernary & ternary = static_cast<const SLTernary &>(*expression);
            const int condition = emitExpression(ternary.condition);
            const int result = temporary(expression->type, expression->storage);
            const int whenTrue = emitExpression(ternary.whenTrue);
            const int whenFalse = emitExpression(ternary.whenFalse);
            const int taken = put(SLOpcode::MASK, -1, condition, -1, expression);
            put(SLOpcode::MOVE, result, whenTrue, -1, expression);
            patch(taken, here());
            put(SLOpcode::POP_MASK, -1, -1, -1, expression);
            const int other = put(SLOpcode::MASK_NOT, -1, condition, -1, expression);
            put(SLOpcode::MOVE, result, whenFalse, -1, expression);
            patch(other, here());
            put(SLOpcode::POP_MASK, -1, -1, -1, expression);
            return result;
        }
        case SLExpression::Kind::CAST:
            return emitCast(expression);
        case SLExpression::Kind::CALL:
            return emitCall(expression);
        case SLExpression::Kind::TUPLE:
        case SLExpression::Kind::INDEX:
            break;
    }
    throw fail("this expression has no instructions yet", expression->line, expression->column);
}

int SLEmitter::emitBinary(const SLExpressionPtr & expression) {
    const SLBinary & binary = static_cast<const SLBinary &>(*expression);
    const int left = emitExpression(binary.left);
    const int right = emitExpression(binary.right);
    const int result = temporary(expression->type, expression->storage);
    put(binaryOpcode(binary.op), result, left, right, expression);
    return result;
}

int SLEmitter::emitCast(const SLExpressionPtr & expression) {
    const SLCast & cast = static_cast<const SLCast &>(*expression);
    const int result = temporary(expression->type, expression->storage);
    if (cast.operand->kind == SLExpression::Kind::TUPLE) {
        // a parenthesised list is a literal for the type in front of it, so each element is
        // moved into its own component of the result
        const SLTuple & tuple = static_cast<const SLTuple &>(*cast.operand);
        for (std::size_t i = 0; i < tuple.elements.size(); i++) {
            const int element = emitExpression(tuple.elements[i]);
            put(SLOpcode::MOVE_COMPONENT, result, element, static_cast<int>(i), expression);
        }
    } else {
        const int operand = emitExpression(cast.operand);
        put(SLOpcode::MOVE, result, operand, -1, expression);
    }
    if (cast.space.empty()) {
        return result;
    }
    // the space is what makes a cast a transform, and which transform it is comes from the
    // type: a point translates, a vector does not, a normal goes by the inverse transpose
    const int space = string(cast.space);
    const int moved = temporary(expression->type, expression->storage);
    put(SLOpcode::TRANSFORM, moved, result, space, expression);
    return moved;
}

int SLEmitter::emitCall(const SLExpressionPtr & expression) {
    const SLCall & call = static_cast<const SLCall &>(*expression);
    if (call.function >= 0) {
        // a shader's own function is inlined rather than called, since a run has no call
        // stack - and the inliner comes with the library's own SL-source functions
        throw fail("'" + call.name + "' is a shader function, which has no instructions yet",
            expression->line, expression->column);
    }
    SLInstruction instruction;
    instruction.opcode = SLOpcode::CALL;
    instruction.target = temporary(expression->type, expression->storage);
    instruction.left = call.signature;
    instruction.line = expression->line;
    instruction.column = expression->column;
    instruction.arguments.reserve(call.arguments.size());
    for (const SLExpressionPtr & argument : call.arguments) {
        instruction.arguments.push_back(emitExpression(argument));
    }
    program_->instructions.push_back(instruction);
    return instruction.target;
}

};  // namespace v3d::render::offline
