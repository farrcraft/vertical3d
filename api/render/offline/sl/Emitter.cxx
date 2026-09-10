/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Emitter.h"

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

Emitter::Emitter(const ShaderPtr & shader, const std::vector<Symbol> & symbols) :
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

int Emitter::put(runtime::Opcode opcode, int target, int left, int right, const ExpressionPtr & where) {
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

void Emitter::emitBlock(const BlockPtr & block) {
    if (!block) {
        return;
    }
    for (const StatementPtr & statement : block->statements) {
        emitStatement(statement);
    }
}

void Emitter::emitStatement(const StatementPtr & statement) {
    if (!statement) {
        return;
    }
    switch (statement->kind) {
        case Statement::Kind::BLOCK:
            emitBlock(boost::static_pointer_cast<Block>(statement));
            return;
        case Statement::Kind::DECLARATION:
            emitDeclaration(statement);
            return;
        case Statement::Kind::ASSIGNMENT:
            emitAssignment(statement);
            return;
        case Statement::Kind::CONDITIONAL:
            emitConditional(statement);
            return;
        case Statement::Kind::WHILE:
            emitWhile(statement);
            return;
        case Statement::Kind::FOR:
            emitFor(statement);
            return;
        case Statement::Kind::JUMP:
            emitJump(statement);
            return;
        case Statement::Kind::EXPRESSION:
            emitExpression(static_cast<const ExpressionStatement &>(*statement).expression);
            return;
        case Statement::Kind::LIGHTING:
            emitLighting(statement);
            return;
    }
}

void Emitter::emitDeclaration(const StatementPtr & statement) {
    const Declaration & declaration = static_cast<const Declaration &>(*statement);
    for (const Declarator & declarator : declaration.declarators) {
        if (!declarator.initialiser) {
            continue;
        }
        const int value = emitExpression(declarator.initialiser);
        put(runtime::Opcode::MOVE, declarator.symbol, value, -1, declarator.initialiser);
    }
}

void Emitter::emitAssignment(const StatementPtr & statement) {
    const Assignment & assignment = static_cast<const Assignment &>(*statement);
    const Variable & variable = static_cast<const Variable &>(*assignment.target);
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

void Emitter::emitConditional(const StatementPtr & statement) {
    const Conditional & conditional = static_cast<const Conditional &>(*statement);
    const int condition = emitExpression(conditional.condition);

    if (conditional.condition->storage != Storage::VARYING) {
        // every point agrees, so the arm not taken costs a branch rather than a pass over
        // the batch with an empty mask
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

    // the points disagree, so both arms run, each under the lanes that took it
    const int taken = put(runtime::Opcode::MASK, -1, condition, -1, conditional.condition);
    emitStatement(conditional.whenTrue);
    patch(taken, here());
    put(runtime::Opcode::POP_MASK, -1, -1, -1, conditional.condition);
    if (!conditional.whenFalse) {
        return;
    }
    const int other = put(runtime::Opcode::MASK_NOT, -1, condition, -1, conditional.condition);
    emitStatement(conditional.whenFalse);
    patch(other, here());
    put(runtime::Opcode::POP_MASK, -1, -1, -1, conditional.condition);
}

void Emitter::emitWhile(const StatementPtr & statement) {
    const While & loop = static_cast<const While &>(*statement);
    emitLoop(loop.condition, loop.body, StatementPtr());
}

void Emitter::emitFor(const StatementPtr & statement) {
    const For & loop = static_cast<const For &>(*statement);
    emitStatement(loop.initialiser);
    emitLoop(loop.condition, loop.body, loop.step);
}

void Emitter::emitLoop(const ExpressionPtr & condition, const StatementPtr & body,
    const StatementPtr & step) {
    /*
        A loop is masked whether its condition varies or not. A uniform condition narrows the
        loop's lanes all together, so it behaves as the jump it would have compiled to, and
        one form means break and continue have one meaning rather than two.
    */
    const int open = put(runtime::Opcode::LOOP, -1, -1, -1, condition);
    const int top = here();
    if (condition) {
        const int test = emitExpression(condition);
        put(runtime::Opcode::LOOP_TEST, -1, test, -1, condition);
    }
    emitStatement(body);
    if (step) {
        // a lane that took a continue comes back for the step: C's continue goes to it
        // rather than past it, and a counter that stopped advancing would never end
        put(runtime::Opcode::LOOP_RESTORE, -1, -1, -1, condition);
        emitStatement(step);
    }
    put(runtime::Opcode::LOOP_END, top, -1, -1, condition);
    patch(open, here());
    put(runtime::Opcode::POP_LOOP, -1, -1, -1, condition);
}

void Emitter::emitJump(const StatementPtr & statement) {
    const Jump & jump = static_cast<const Jump &>(*statement);
    switch (jump.where) {
        case Jump::Where::BREAK:
            put(runtime::Opcode::BREAK, -1, -1, -1, ExpressionPtr());
            return;
        case Jump::Where::CONTINUE:
            put(runtime::Opcode::CONTINUE, -1, -1, -1, ExpressionPtr());
            return;
        case Jump::Where::RETURN:
            if (jump.value && !returns_.empty()) {
                const int value = emitExpression(jump.value);
                put(runtime::Opcode::MOVE, returns_.back(), value, -1, jump.value);
            }
            put(runtime::Opcode::RETURN, -1, -1, -1, jump.value);
            return;
    }
}

int Emitter::emitExpression(const ExpressionPtr & expression) {
    switch (expression->kind) {
        case Expression::Kind::NUMBER:
            return number(static_cast<const Number &>(*expression).value);
        case Expression::Kind::STRING:
            return string(static_cast<const String &>(*expression).value);
        case Expression::Kind::VARIABLE:
            return static_cast<const Variable &>(*expression).symbol;
        case Expression::Kind::UNARY: {
            const Unary & unary = static_cast<const Unary &>(*expression);
            const int operand = emitExpression(unary.operand);
            const int result = temporary(expression->type, expression->storage);
            put(unary.op == "!" ? runtime::Opcode::NOT : runtime::Opcode::NEGATE, result, operand, -1, expression);
            return result;
        }
        case Expression::Kind::BINARY:
            return emitBinary(expression);
        case Expression::Kind::TERNARY: {
            // both arms are computed and one is chosen, which is what a mask would do with
            // fewer instructions and the same work
            const Ternary & ternary = static_cast<const Ternary &>(*expression);
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
        case Expression::Kind::CAST:
            return emitCast(expression);
        case Expression::Kind::CALL:
            return emitCall(expression);
        case Expression::Kind::TUPLE:
        case Expression::Kind::INDEX:
            break;
    }
    throw fail("this expression has no instructions yet", expression->line, expression->column);
}

int Emitter::emitBinary(const ExpressionPtr & expression) {
    const Binary & binary = static_cast<const Binary &>(*expression);
    const int left = emitExpression(binary.left);
    const int right = emitExpression(binary.right);
    const int result = temporary(expression->type, expression->storage);
    put(binaryOpcode(binary.op), result, left, right, expression);
    return result;
}

int Emitter::emitCast(const ExpressionPtr & expression) {
    const Cast & cast = static_cast<const Cast &>(*expression);
    const int result = temporary(expression->type, expression->storage);
    if (cast.operand->kind == Expression::Kind::TUPLE) {
        // a parenthesised list is a literal for the type in front of it, so each element is
        // moved into its own component of the result
        const Tuple & tuple = static_cast<const Tuple &>(*cast.operand);
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
    // the space is what makes a cast a transform, and which transform it is comes from the
    // type: a point translates, a vector does not, a normal goes by the inverse transpose
    const int space = string(cast.space);
    const int moved = temporary(expression->type, expression->storage);
    put(runtime::Opcode::TRANSFORM, moved, result, space, expression);
    return moved;
}

int Emitter::emitCall(const ExpressionPtr & expression) {
    const Call & call = static_cast<const Call &>(*expression);
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
    for (const ExpressionPtr & argument : call.arguments) {
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

void Emitter::emitLighting(const StatementPtr & statement) {
    const Lighting & lighting = static_cast<const Lighting &>(*statement);
    ExpressionPtr where;
    std::vector<int> given;
    given.reserve(lighting.arguments.size());
    for (const ExpressionPtr & argument : lighting.arguments) {
        given.push_back(emitExpression(argument));
        where = argument;
    }

    runtime::Instruction open;
    open.arguments = given;
    open.line = statement->line;
    open.column = statement->column;
    if (lighting.construct != Lighting::Construct::ILLUMINANCE) {
        /*
            A light shader's end of the message passing. L and Ps are its own globals: the
            construct writes the first for every point it lights and reads the second to
            know where each of those points is.
        */
        open.opcode = lighting.construct == Lighting::Construct::ILLUMINATE
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

int Emitter::emitInline(const ExpressionPtr & expression) {
    const Call & call = static_cast<const Call &>(*expression);
    const Function & function = shader_->functions[static_cast<std::size_t>(call.function)];
    // every argument is evaluated before any formal is written, so that an argument which
    // is itself a call cannot land on a formal this one has already filled
    std::vector<int> given;
    given.reserve(call.arguments.size());
    for (const ExpressionPtr & argument : call.arguments) {
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
