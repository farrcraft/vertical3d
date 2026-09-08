/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SLCompiler.h"

#include <algorithm>
#include <string>
#include <vector>

#include "SLBuiltins.h"
#include "SLTypes.h"

namespace v3d::render::offline {

namespace {

typedef SLSignature::Argument Argument;

const unsigned int SURFACE = 1u << 0;
const unsigned int LIGHT = 1u << 1;
const unsigned int IMAGER = 1u << 2;

/**
 * A shader global: which shader types have it, and which of them may write it.
 *
 * The three lists are the standard's. A name that is not here is not a global, so a light
 * shader mentioning `Ci` is told that `Ci` belongs to a surface and an imager rather than
 * being told it is undeclared - which is a far more useful thing to read.
 **/
class Global final {
 public:
    const char* name;
    SLType type;
    SLStorage storage;
    unsigned int shaders;
    unsigned int writable;
    /**
     * Only meaningful inside a lighting construct. A surface shader's L and Cl are set by
     * the light whose shader illuminance is running, and mean nothing outside that body.
     **/
    bool lighting;
};

const Global GLOBALS[] = {
    { "P", SLType::POINT, SLStorage::VARYING, SURFACE | LIGHT | IMAGER, 0, false },
    { "N", SLType::NORMAL, SLStorage::VARYING, SURFACE, 0, false },
    { "Ng", SLType::NORMAL, SLStorage::VARYING, SURFACE, 0, false },
    { "I", SLType::VECTOR, SLStorage::VARYING, SURFACE, 0, false },
    { "E", SLType::POINT, SLStorage::UNIFORM, SURFACE, 0, false },
    { "Cs", SLType::COLOR, SLStorage::VARYING, SURFACE, 0, false },
    { "Os", SLType::COLOR, SLStorage::VARYING, SURFACE, 0, false },
    { "s", SLType::FLOAT, SLStorage::VARYING, SURFACE, 0, false },
    { "t", SLType::FLOAT, SLStorage::VARYING, SURFACE, 0, false },
    { "u", SLType::FLOAT, SLStorage::VARYING, SURFACE, 0, false },
    { "v", SLType::FLOAT, SLStorage::VARYING, SURFACE, 0, false },
    { "du", SLType::FLOAT, SLStorage::VARYING, SURFACE, 0, false },
    { "dv", SLType::FLOAT, SLStorage::VARYING, SURFACE, 0, false },
    { "Ci", SLType::COLOR, SLStorage::VARYING, SURFACE | IMAGER, SURFACE | IMAGER, false },
    { "Oi", SLType::COLOR, SLStorage::VARYING, SURFACE | IMAGER, SURFACE | IMAGER, false },
    { "Ps", SLType::POINT, SLStorage::VARYING, LIGHT, 0, false },
    { "alpha", SLType::FLOAT, SLStorage::VARYING, IMAGER, 0, false },
    // a light writes these; a surface reads them, and only inside an illuminance body
    { "L", SLType::VECTOR, SLStorage::VARYING, SURFACE | LIGHT, LIGHT, true },
    { "Cl", SLType::COLOR, SLStorage::VARYING, SURFACE | LIGHT, LIGHT, true }
};

unsigned int bit(SLShaderType type) {
    switch (type) {
        case SLShaderType::SURFACE:
            return SURFACE;
        case SLShaderType::LIGHT:
            return LIGHT;
        case SLShaderType::IMAGER:
            return IMAGER;
        case SLShaderType::DISPLACEMENT:
        case SLShaderType::VOLUME:
            return 0;
    }
    return 0;
}

/**
 * Which shader types a global belongs to, as prose for a diagnostic.
 **/
std::string owners(unsigned int shaders) {
    std::vector<std::string> names;
    if ((shaders & SURFACE) != 0) {
        names.push_back("a surface");
    }
    if ((shaders & LIGHT) != 0) {
        names.push_back("a light");
    }
    if ((shaders & IMAGER) != 0) {
        names.push_back("an imager");
    }
    std::string text;
    for (std::size_t i = 0; i < names.size(); i++) {
        if (i > 0) {
            text += i + 1 == names.size() ? " and " : ", ";
        }
        text += names[i];
    }
    return text;
}

const char* construct(SLLighting::Construct which) {
    switch (which) {
        case SLLighting::Construct::ILLUMINANCE:
            return "illuminance";
        case SLLighting::Construct::ILLUMINATE:
            return "illuminate";
        case SLLighting::Construct::SOLAR:
            return "solar";
    }
    return "";
}

std::string article(SLShaderType type) {
    return type == SLShaderType::IMAGER ? "an " + std::string(name(type)) : "a " + std::string(name(type));
}

bool accepts(Argument wanted, SLType given) {
    switch (wanted) {
        case Argument::ANY:
            return given != SLType::VOID;
        case Argument::NUMBER:
            return given == SLType::FLOAT || given == SLType::COLOR || pointlike(given);
        case Argument::POINTLIKE:
            // a float replicates into a direction, which is what "normalize(0)" leans on
            return pointlike(given) || given == SLType::FLOAT;
        case Argument::FLOAT:
            return coercible(given, SLType::FLOAT);
        case Argument::POINT:
            return coercible(given, SLType::POINT);
        case Argument::VECTOR:
            return coercible(given, SLType::VECTOR);
        case Argument::NORMAL:
            return coercible(given, SLType::NORMAL);
        case Argument::COLOR:
            return coercible(given, SLType::COLOR);
        case Argument::MATRIX:
            return coercible(given, SLType::MATRIX);
        case Argument::STRING:
            return given == SLType::STRING;
    }
    return false;
}

/**
 * Whether one way of calling a standard library function takes these argument types.
 **/
bool suits(const SLSignature & signature, const std::vector<SLType> & given) {
    const bool arity = signature.variadic ?
        given.size() >= signature.arguments.size() : given.size() == signature.arguments.size();
    if (!arity) {
        return false;
    }
    for (std::size_t i = 0; i < signature.arguments.size(); i++) {
        if (!accepts(signature.arguments[i], given[i])) {
            return false;
        }
    }
    return true;
}

SLStorage join(SLStorage left, SLStorage right) {
    return left == SLStorage::VARYING || right == SLStorage::VARYING ?
        SLStorage::VARYING : SLStorage::UNIFORM;
}

};  // namespace

SLCompiler::SLCompiler(const SLShaderPtr & shader) : shader_(shader) {
}

const std::string & SLCompiler::error() const {
    return error_;
}

const std::vector<SLSymbol> & SLCompiler::symbols() const {
    return symbols_;
}

SLCompiler::Failure SLCompiler::fail(const std::string & message, unsigned int line, unsigned int column) {
    if (error_.empty()) {
        error_ = message + " at line " + std::to_string(line) + ", column " + std::to_string(column);
    }
    return Failure();
}

int SLCompiler::declare(const std::string & name, SLType type, SLStorage storage,
    SLSymbol::Role role, bool writable, unsigned int line, unsigned int column) {
    // a local shadows whatever is outside its block; a parameter that collides with a global
    // or with another parameter is a shader saying two things by one name
    if (role == SLSymbol::Role::PARAMETER) {
        const int existing = lookup(name);
        if (existing >= 0) {
            const bool global = symbols_[static_cast<std::size_t>(existing)].role == SLSymbol::Role::GLOBAL;
            throw fail("'" + name + "' is already " + (global ? "a shader global" : "a parameter"),
                line, column);
        }
    }
    SLSymbol symbol;
    symbol.name = name;
    symbol.type = type;
    symbol.storage = storage == SLStorage::UNSPECIFIED ? SLStorage::UNIFORM : storage;
    symbol.declared = storage != SLStorage::UNSPECIFIED;
    symbol.role = role;
    symbol.writable = writable;
    symbols_.push_back(symbol);

    Binding binding;
    binding.name = name;
    binding.symbol = static_cast<int>(symbols_.size()) - 1;
    scope_.push_back(binding);
    return binding.symbol;
}

int SLCompiler::lookup(const std::string & name) const {
    // innermost first, so an inner declaration shadows an outer one
    for (std::size_t i = scope_.size(); i > 0; i--) {
        if (scope_[i - 1].name == name) {
            return scope_[i - 1].symbol;
        }
    }
    return -1;
}

void SLCompiler::declareGlobals() {
    const unsigned int mine = bit(shader_->type);
    for (const Global & global : GLOBALS) {
        if ((global.shaders & mine) == 0) {
            continue;
        }
        const int index = declare(global.name, global.type, global.storage, SLSymbol::Role::GLOBAL,
            (global.writable & mine) != 0, shader_->line, shader_->column);
        if (global.lighting && (global.writable & mine) == 0) {
            lighting_.push_back(index);
        }
    }
}

void SLCompiler::declareParameters() {
    for (SLParameter & parameter : shader_->parameters) {
        if (parameter.type == SLType::VOID) {
            throw fail("a parameter cannot be void", parameter.line, parameter.column);
        }
        // a parameter is uniform unless it says otherwise: a scene binds one value for the
        // whole primitive, and only a declaration can say the renderer will vary it
        const SLType given = checkExpression(parameter.defaultValue);
        if (!coercible(given, parameter.type)) {
            throw fail(std::string("the default for '") + parameter.name + "' is " + name(given) +
                ", which is not " + name(parameter.type), parameter.line, parameter.column);
        }
        parameter.symbol = declare(parameter.name, parameter.type, parameter.storage,
            SLSymbol::Role::PARAMETER, true, parameter.line, parameter.column);
        symbols_[parameter.symbol].output = parameter.output;
    }
}

bool SLCompiler::compile() {
    try {
        if (!shader_->supported()) {
            throw fail(std::string("a ") + name(shader_->type) + " shader is not supported",
                shader_->line, shader_->column);
        }
        declareGlobals();
        declareParameters();
        results_.assign(shader_->functions.size(), SLStorage::UNIFORM);
        calls_.assign(shader_->functions.size(), std::vector<int>());
        checkFunctions();
        checkBlock(shader_->body);
        checkCallGraph();
        infer();
        if (!violation_.empty() && error_.empty()) {
            error_ = violation_;
            return false;
        }
    } catch (const Failure &) {
        return false;
    }
    return error_.empty();
}

void SLCompiler::checkFunctions() {
    for (std::size_t i = 0; i < shader_->functions.size(); i++) {
        SLFunction & function = shader_->functions[i];
        const std::size_t mark = scope_.size();
        inside_ = static_cast<int>(i);
        for (SLParameter & formal : function.parameters) {
            if (formal.type == SLType::VOID) {
                throw fail("a parameter cannot be void", formal.line, formal.column);
            }
            formal.symbol = declare(formal.name, formal.type, formal.storage,
                SLSymbol::Role::LOCAL, true, formal.line, formal.column);
        }
        checkBlock(function.body);
        scope_.resize(mark);
    }
    inside_ = -1;
}

void SLCompiler::checkCallGraph() {
    // the machine has a register file per shader run and no call stack, so a recursive
    // shader has no meaning to give. Depth first over the call graph, colouring as it goes
    const std::size_t count = calls_.size();
    std::vector<int> colour(count, 0);
    std::vector<std::size_t> stack;
    for (std::size_t root = 0; root < count; root++) {
        if (colour[root] != 0) {
            continue;
        }
        stack.push_back(root);
        while (!stack.empty()) {
            const std::size_t current = stack.back();
            if (colour[current] == 0) {
                colour[current] = 1;
                for (int called : calls_[current]) {
                    const std::size_t next = static_cast<std::size_t>(called);
                    if (colour[next] == 1) {
                        const SLFunction & function = shader_->functions[next];
                        throw fail("'" + function.name + "' calls itself, and a shader run has no call stack",
                            function.line, function.column);
                    }
                    if (colour[next] == 0) {
                        stack.push_back(next);
                    }
                }
            } else {
                colour[current] = 2;
                stack.pop_back();
            }
        }
    }
}

void SLCompiler::checkBlock(const SLBlockPtr & block) {
    if (!block) {
        return;
    }
    const std::size_t mark = scope_.size();
    for (const SLStatementPtr & statement : block->statements) {
        checkStatement(statement);
    }
    // a local goes out of scope with its block, and its symbol stays: the machine allocates
    // against the symbol, and two locals of the same name in sibling blocks are two of them
    scope_.resize(mark);
}

void SLCompiler::checkStatement(const SLStatementPtr & statement) {
    if (!statement) {
        return;
    }
    switch (statement->kind) {
        case SLStatement::Kind::BLOCK:
            checkBlock(boost::static_pointer_cast<SLBlock>(statement));
            return;
        case SLStatement::Kind::DECLARATION:
            checkDeclaration(statement);
            return;
        case SLStatement::Kind::ASSIGNMENT:
            checkAssignment(statement);
            return;
        case SLStatement::Kind::CONDITIONAL: {
            const SLConditional & conditional = static_cast<const SLConditional &>(*statement);
            checkCondition(conditional.condition, "if");
            checkStatement(conditional.whenTrue);
            checkStatement(conditional.whenFalse);
            return;
        }
        case SLStatement::Kind::WHILE: {
            const SLWhile & loop = static_cast<const SLWhile &>(*statement);
            checkCondition(loop.condition, "while");
            checkStatement(loop.body);
            return;
        }
        case SLStatement::Kind::FOR: {
            const SLFor & loop = static_cast<const SLFor &>(*statement);
            checkStatement(loop.initialiser);
            if (loop.condition) {
                checkCondition(loop.condition, "for");
            }
            checkStatement(loop.step);
            checkStatement(loop.body);
            return;
        }
        case SLStatement::Kind::JUMP:
            checkJump(statement);
            return;
        case SLStatement::Kind::EXPRESSION:
            checkExpression(static_cast<const SLExpressionStatement &>(*statement).expression);
            return;
        case SLStatement::Kind::LIGHTING:
            checkLighting(statement);
            return;
    }
}

void SLCompiler::checkCondition(const SLExpressionPtr & condition, const char* construct) {
    const SLType type = checkExpression(condition);
    if (!coercible(type, SLType::FLOAT)) {
        throw fail(std::string("the condition of a '") + construct + "' is " + name(type) +
            ", which is not a number", condition->line, condition->column);
    }
}

void SLCompiler::checkJump(const SLStatementPtr & statement) {
    const SLJump & jump = static_cast<const SLJump &>(*statement);
    if (!jump.value) {
        return;
    }
    if (inside_ < 0) {
        throw fail("a shader returns no value", statement->line, statement->column);
    }
    const SLFunction & function = shader_->functions[static_cast<std::size_t>(inside_)];
    const SLType given = checkExpression(jump.value);
    if (!coercible(given, function.type)) {
        throw fail("'" + function.name + "' returns " + name(function.type) + ", not " + name(given),
            statement->line, statement->column);
    }
}

void SLCompiler::checkDeclaration(const SLStatementPtr & statement) {
    SLDeclaration & declaration = static_cast<SLDeclaration &>(*statement);
    if (declaration.type == SLType::VOID) {
        throw fail("a variable cannot be void", statement->line, statement->column);
    }
    for (SLDeclarator & declarator : declaration.declarators) {
        if (declarator.initialiser) {
            // checked before the name is declared, so "float x = x" reads the outer x
            const SLType given = checkExpression(declarator.initialiser);
            if (!coercible(given, declaration.type)) {
                throw fail("'" + declarator.name + "' is " + name(declaration.type) +
                    " and is given " + name(given), declarator.line, declarator.column);
            }
        }
        declarator.symbol = declare(declarator.name, declaration.type, declaration.storage,
            SLSymbol::Role::LOCAL, true, declarator.line, declarator.column);
    }
}

void SLCompiler::checkAssignment(const SLStatementPtr & statement) {
    const SLAssignment & assignment = static_cast<const SLAssignment &>(*statement);
    if (assignment.target->kind != SLExpression::Kind::VARIABLE) {
        throw fail("only a variable can be assigned to",
            assignment.target->line, assignment.target->column);
    }
    const SLType target = checkExpression(assignment.target);
    const SLVariable & variable = static_cast<const SLVariable &>(*assignment.target);
    const SLSymbol & symbol = symbols_[static_cast<std::size_t>(variable.symbol)];
    if (!symbol.writable) {
        const std::string mine = article(shader_->type);
        if (symbol.role == SLSymbol::Role::GLOBAL) {
            throw fail("'" + variable.name + "' cannot be assigned in " + mine + " shader",
                assignment.target->line, assignment.target->column);
        }

        throw fail("'" + variable.name + "' cannot be assigned",
            assignment.target->line, assignment.target->column);
    }
    const SLType given = checkExpression(assignment.value);
    // a compound form is the operator and then the assignment, so both have to work
    const SLType combined = assignment.op == "=" ? given : arithmetic(target, given);
    if (combined == SLType::VOID || !coercible(combined, target)) {
        throw fail("'" + variable.name + "' is " + name(target) + " and is given " + name(given),
            statement->line, statement->column);
    }
}

void SLCompiler::checkLighting(const SLStatementPtr & statement) {
    SLLighting & lighting = static_cast<SLLighting &>(*statement);
    const bool surface = shader_->type == SLShaderType::SURFACE;
    if (lighting.construct == SLLighting::Construct::ILLUMINANCE) {
        if (!surface) {
            throw fail("'illuminance' is only valid in a surface shader",
                statement->line, statement->column);
        }
        if (lighting.arguments.empty()) {
            throw fail("'illuminance' needs the point being shaded", statement->line, statement->column);
        }
    } else if (shader_->type != SLShaderType::LIGHT) {
        throw fail(std::string("'") + construct(lighting.construct) + "' is only valid in a light shader",
            statement->line, statement->column);
    }
    for (const SLExpressionPtr & argument : lighting.arguments) {
        checkExpression(argument);
    }
    depth_++;
    checkStatement(lighting.body);
    depth_--;
}

SLType SLCompiler::checkExpression(const SLExpressionPtr & expression) {
    if (!expression) {
        return SLType::VOID;
    }
    switch (expression->kind) {
        case SLExpression::Kind::NUMBER:
            expression->type = SLType::FLOAT;
            return expression->type;
        case SLExpression::Kind::STRING:
            expression->type = SLType::STRING;
            return expression->type;
        case SLExpression::Kind::VARIABLE:
            return checkVariable(expression);
        case SLExpression::Kind::CALL:
            return checkCall(expression);
        case SLExpression::Kind::UNARY:
            return checkUnary(expression);
        case SLExpression::Kind::BINARY:
            return checkBinary(expression);
        case SLExpression::Kind::TERNARY:
            return checkTernary(expression);
        case SLExpression::Kind::CAST:
            return checkCast(expression);
        case SLExpression::Kind::TUPLE:
            // a parenthesised list is a literal for whatever a cast says it is, and means
            // nothing on its own - checkCast is the only place that reads one
            throw fail("a parenthesised list of values needs a type in front of it",
                expression->line, expression->column);
        case SLExpression::Kind::INDEX:
            throw fail("an array is not supported", expression->line, expression->column);
    }
    return SLType::VOID;
}

SLType SLCompiler::checkVariable(const SLExpressionPtr & expression) {
    SLVariable & variable = static_cast<SLVariable &>(*expression);
    variable.symbol = lookup(variable.name);
    if (variable.symbol < 0) {
        // a global of another shader type is a far more useful thing to be told about than
        // an undeclared name
        for (const Global & global : GLOBALS) {
            if (variable.name == global.name) {
                throw fail("'" + variable.name + "' belongs to " + owners(global.shaders) +
                    " shader, not to " + article(shader_->type) + " shader",
                    expression->line, expression->column);
            }
        }
        throw fail("'" + variable.name + "' is not declared", expression->line, expression->column);
    }
    if (depth_ == 0) {
        for (int index : lighting_) {
            if (index == variable.symbol) {
                throw fail("'" + variable.name + "' is set by a light and means nothing outside an "
                    "'illuminance' body", expression->line, expression->column);
            }
        }
    }
    expression->type = symbols_[static_cast<std::size_t>(variable.symbol)].type;
    return expression->type;
}

SLType SLCompiler::checkUnary(const SLExpressionPtr & expression) {
    const SLUnary & unary = static_cast<const SLUnary &>(*expression);
    const SLType operand = checkExpression(unary.operand);
    if (unary.op == "!") {
        if (!coercible(operand, SLType::FLOAT)) {
            throw fail("'!' takes a number, not " + std::string(name(operand)),
                expression->line, expression->column);
        }
        expression->type = SLType::FLOAT;
        return expression->type;
    }
    if (operand == SLType::STRING || operand == SLType::VOID) {
        throw fail("'-' takes a number, not " + std::string(name(operand)),
            expression->line, expression->column);
    }
    expression->type = operand;
    return expression->type;
}

SLType SLCompiler::checkBinary(const SLExpressionPtr & expression) {
    const SLBinary & binary = static_cast<const SLBinary &>(*expression);
    const SLType left = checkExpression(binary.left);
    const SLType right = checkExpression(binary.right);
    const std::string & op = binary.op;

    if (op == "." || op == "^") {
        // the two that read as something else: a dot product and a cross product, over
        // positions and directions rather than over anything with three of something
        if (!accepts(Argument::POINTLIKE, left) || !accepts(Argument::POINTLIKE, right)) {
            throw fail("'" + op + "' takes two positions or directions, not " +
                name(left) + " and " + name(right), expression->line, expression->column);
        }
        expression->type = op == "." ? SLType::FLOAT : SLType::VECTOR;
        return expression->type;
    }
    if (op == "&&" || op == "||") {
        if (!coercible(left, SLType::FLOAT) || !coercible(right, SLType::FLOAT)) {
            throw fail("'" + op + "' takes numbers, not " + name(left) + " and " + name(right),
                expression->line, expression->column);
        }
        expression->type = SLType::FLOAT;
        return expression->type;
    }
    if (op == "<" || op == "<=" || op == ">" || op == ">=") {
        if (!coercible(left, SLType::FLOAT) || !coercible(right, SLType::FLOAT)) {
            throw fail("'" + op + "' compares numbers, not " + name(left) + " and " + name(right),
                expression->line, expression->column);
        }
        expression->type = SLType::FLOAT;
        return expression->type;
    }
    if (op == "==" || op == "!=") {
        if (!coercible(left, right) && !coercible(right, left)) {
            throw fail("'" + op + "' cannot compare " + name(left) + " with " + name(right),
                expression->line, expression->column);
        }
        expression->type = SLType::FLOAT;
        return expression->type;
    }
    const SLType combined = arithmetic(left, right);
    if (combined == SLType::VOID) {
        throw fail("'" + op + "' has no meaning between " + name(left) + " and " + name(right),
            expression->line, expression->column);
    }
    expression->type = combined;
    return expression->type;
}

SLType SLCompiler::checkTernary(const SLExpressionPtr & expression) {
    const SLTernary & ternary = static_cast<const SLTernary &>(*expression);
    checkCondition(ternary.condition, "?:");
    const SLType whenTrue = checkExpression(ternary.whenTrue);
    const SLType whenFalse = checkExpression(ternary.whenFalse);
    if (coercible(whenTrue, whenFalse)) {
        expression->type = whenFalse;
    } else if (coercible(whenFalse, whenTrue)) {
        expression->type = whenTrue;
    } else {
        throw fail("the arms of a '?:' are " + std::string(name(whenTrue)) + " and " + name(whenFalse),
            expression->line, expression->column);
    }
    return expression->type;
}

SLType SLCompiler::checkCast(const SLExpressionPtr & expression) {
    const SLCast & cast = static_cast<const SLCast &>(*expression);
    if (cast.type == SLType::VOID) {
        throw fail("nothing can be cast to void", expression->line, expression->column);
    }
    if (!cast.space.empty() && cast.type == SLType::FLOAT) {
        throw fail("a coordinate space means nothing to a float", expression->line, expression->column);
    }
    if (cast.operand->kind == SLExpression::Kind::TUPLE) {
        // a parenthesised list is a literal for the type in front of it: three floats are a
        // point or a colour and sixteen are a matrix
        const SLTuple & tuple = static_cast<const SLTuple &>(*cast.operand);
        const unsigned int wanted = components(cast.type);
        if (tuple.elements.size() != wanted) {
            throw fail(std::string(name(cast.type)) + " is " + std::to_string(wanted) +
                " values, and " + std::to_string(tuple.elements.size()) + " were given",
                cast.operand->line, cast.operand->column);
        }
        for (const SLExpressionPtr & element : tuple.elements) {
            const SLType given = checkExpression(element);
            if (!coercible(given, SLType::FLOAT)) {
                throw fail("a value of " + std::string(name(cast.type)) + " is a number, not " + name(given),
                    element->line, element->column);
            }
        }
        cast.operand->type = cast.type;
        expression->type = cast.type;
        return expression->type;
    }
    const SLType given = checkExpression(cast.operand);
    if (!coercible(given, cast.type)) {
        throw fail(std::string(name(given)) + " cannot be cast to " + name(cast.type),
            expression->line, expression->column);
    }
    expression->type = cast.type;
    return expression->type;
}

SLType SLCompiler::checkCall(const SLExpressionPtr & expression) {
    SLCall & call = static_cast<SLCall &>(*expression);
    std::vector<SLType> given;
    given.reserve(call.arguments.size());
    for (const SLExpressionPtr & argument : call.arguments) {
        given.push_back(checkExpression(argument));
    }
    // a shader's own function wins over a standard one of the same name, which is how a
    // shader replaces a light model it does not like
    const int function = checkShaderCall(call, given);
    if (function >= 0) {
        call.function = function;
        if (inside_ >= 0) {
            calls_[static_cast<std::size_t>(inside_)].push_back(function);
        }
        expression->type = shader_->functions[static_cast<std::size_t>(function)].type;
        return expression->type;
    }
    return checkBuiltinCall(call, given);
}

int SLCompiler::checkShaderCall(SLCall & call, const std::vector<SLType> & given) {
    for (std::size_t i = 0; i < shader_->functions.size(); i++) {
        const SLFunction & function = shader_->functions[i];
        if (function.name != call.name) {
            continue;
        }
        if (function.parameters.size() != given.size()) {
            throw fail("'" + call.name + "' takes " + std::to_string(function.parameters.size()) +
                " arguments, and " + std::to_string(given.size()) + " were given",
                call.line, call.column);
        }
        for (std::size_t argument = 0; argument < given.size(); argument++) {
            if (!coercible(given[argument], function.parameters[argument].type)) {
                throw fail("argument " + std::to_string(argument + 1) + " of '" + call.name +
                    "' is " + name(function.parameters[argument].type) + ", not " + name(given[argument]),
                    call.arguments[argument]->line, call.arguments[argument]->column);
            }
        }
        return static_cast<int>(i);
    }
    return -1;
}

SLType SLCompiler::checkBuiltinCall(SLCall & call, const std::vector<SLType> & given) {
    const std::vector<SLSignature> & table = builtins();
    bool named = false;
    for (std::size_t index = 0; index < table.size(); index++) {
        const SLSignature & signature = table[index];
        if (signature.name != call.name) {
            continue;
        }
        named = true;
        if (!suits(signature, given)) {
            continue;
        }
        call.signature = static_cast<int>(index);
        call.type = signature.resultFrom >= 0 ?
            given[static_cast<std::size_t>(signature.resultFrom)] : signature.result;
        return call.type;
    }
    if (!named) {
        throw fail("'" + call.name + "' is not a function", call.line, call.column);
    }
    throw fail("'" + call.name + "' cannot be called with those arguments", call.line, call.column);
}

bool SLCompiler::escapes(const SLStatementPtr & loop) const {
    return std::ranges::find(escaping_, loop.get()) != escaping_.end();
}

void SLCompiler::mark(const SLStatement* loop) {
    if (std::ranges::find(escaping_, loop) != escaping_.end()) {
        return;
    }
    escaping_.push_back(loop);
    changed_ = true;
}

void SLCompiler::spread(int symbol, const SLExpressionPtr & from) {
    SLSymbol & entry = symbols_[static_cast<std::size_t>(symbol)];
    if (entry.storage == SLStorage::VARYING) {
        return;
    }
    if (entry.declared) {
        // an explicit uniform that a varying value reaches is a shader saying two things at
        // once, and quietly keeping one of them is how a grid comes out with one point's
        // answer
        if (violation_.empty()) {
            const unsigned int line = from ? from->line : shader_->line;
            const unsigned int column = from ? from->column : shader_->column;
            violation_ = "'" + entry.name + "' is uniform and is given a varying value at line " +
                std::to_string(line) + ", column " + std::to_string(column);
        }
        return;
    }
    entry.storage = SLStorage::VARYING;
    changed_ = true;
}

void SLCompiler::infer() {
    // a loop can carry a varying value back to a name that was read before it was written,
    // so one pass is not enough. Nothing ever moves from varying back to uniform, so the
    // walk is monotone and settles
    for (int round = 0; round < 64; round++) {
        changed_ = false;
        for (std::size_t i = 0; i < shader_->functions.size(); i++) {
            inside_ = static_cast<int>(i);
            inferBlock(shader_->functions[i].body, false);
        }
        inside_ = -1;
        inferBlock(shader_->body, false);
        if (!changed_) {
            return;
        }
    }
}

void SLCompiler::inferBlock(const SLBlockPtr & block, bool varyingContext) {
    if (!block) {
        return;
    }
    for (const SLStatementPtr & statement : block->statements) {
        inferStatement(statement, varyingContext);
    }
}

void SLCompiler::inferStatement(const SLStatementPtr & statement, bool varyingContext) {
    if (!statement) {
        return;
    }
    switch (statement->kind) {
        case SLStatement::Kind::BLOCK:
            inferBlock(boost::static_pointer_cast<SLBlock>(statement), varyingContext);
            return;
        case SLStatement::Kind::DECLARATION:
            inferDeclaration(statement, varyingContext);
            return;
        case SLStatement::Kind::ASSIGNMENT:
            inferAssignment(statement, varyingContext);
            return;
        case SLStatement::Kind::CONDITIONAL: {
            const SLConditional & conditional = static_cast<const SLConditional &>(*statement);
            const bool varying = inferExpression(conditional.condition) == SLStorage::VARYING;
            inferStatement(conditional.whenTrue, varyingContext || varying);
            inferStatement(conditional.whenFalse, varyingContext || varying);
            return;
        }
        case SLStatement::Kind::WHILE: {
            const SLWhile & loop = static_cast<const SLWhile &>(*statement);
            const bool varying = inferExpression(loop.condition) == SLStorage::VARYING;
            enclosing_.push_back(statement.get());
            inferStatement(loop.body, varyingContext || varying || escapes(statement));
            enclosing_.pop_back();
            return;
        }
        case SLStatement::Kind::FOR: {
            const SLFor & loop = static_cast<const SLFor &>(*statement);
            inferStatement(loop.initialiser, varyingContext);
            const bool varying = loop.condition ?
                inferExpression(loop.condition) == SLStorage::VARYING : false;
            const bool inside = varyingContext || varying || escapes(statement);
            enclosing_.push_back(statement.get());
            inferStatement(loop.step, inside);
            inferStatement(loop.body, inside);
            enclosing_.pop_back();
            return;
        }
        case SLStatement::Kind::JUMP:
            inferJump(statement, varyingContext);
            return;
        case SLStatement::Kind::EXPRESSION:
            inferExpression(static_cast<const SLExpressionStatement &>(*statement).expression);
            return;
        case SLStatement::Kind::LIGHTING: {
            const SLLighting & lighting = static_cast<const SLLighting &>(*statement);
            for (const SLExpressionPtr & argument : lighting.arguments) {
                inferExpression(argument);
            }
            // the body runs once per light with L and Cl set per point, so everything it
            // writes is varying whatever reached it
            inferStatement(lighting.body, true);
            return;
        }
    }
}

void SLCompiler::inferDeclaration(const SLStatementPtr & statement, bool varyingContext) {
    const SLDeclaration & declaration = static_cast<const SLDeclaration &>(*statement);
    for (const SLDeclarator & declarator : declaration.declarators) {
        const SLStorage value = declarator.initialiser ?
            inferExpression(declarator.initialiser) : SLStorage::UNIFORM;
        if (varyingContext || value == SLStorage::VARYING) {
            spread(declarator.symbol, declarator.initialiser);
        }
    }
}

void SLCompiler::inferAssignment(const SLStatementPtr & statement, bool varyingContext) {
    const SLAssignment & assignment = static_cast<const SLAssignment &>(*statement);
    const SLStorage value = inferExpression(assignment.value);
    inferExpression(assignment.target);
    const SLVariable & variable = static_cast<const SLVariable &>(*assignment.target);
    // different points take different arms, so anything written under a varying condition is
    // varying whatever was written to it
    if (varyingContext || value == SLStorage::VARYING) {
        spread(variable.symbol, assignment.value);
    }
}

void SLCompiler::inferJump(const SLStatementPtr & statement, bool varyingContext) {
    const SLJump & jump = static_cast<const SLJump &>(*statement);
    if (jump.where != SLJump::Where::RETURN) {
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
    const SLStorage value = inferExpression(jump.value);
    SLStorage & result = results_[static_cast<std::size_t>(inside_)];
    if (result != SLStorage::VARYING && (varyingContext || value == SLStorage::VARYING)) {
        result = SLStorage::VARYING;
        changed_ = true;
    }
}

SLStorage SLCompiler::inferExpression(const SLExpressionPtr & expression) {
    if (!expression) {
        return SLStorage::UNIFORM;
    }
    SLStorage storage = SLStorage::UNIFORM;
    switch (expression->kind) {
        case SLExpression::Kind::NUMBER:
        case SLExpression::Kind::STRING:
            // a string names a coordinate space, a texture or a message, and there is no
            // per-point one to name: allowing one would make every transform a runtime
            // string lookup
            storage = SLStorage::UNIFORM;
            break;
        case SLExpression::Kind::VARIABLE:
            storage = symbols_[static_cast<std::size_t>(
                static_cast<const SLVariable &>(*expression).symbol)].storage;
            break;
        case SLExpression::Kind::UNARY:
            storage = inferExpression(static_cast<const SLUnary &>(*expression).operand);
            break;
        case SLExpression::Kind::BINARY: {
            const SLBinary & binary = static_cast<const SLBinary &>(*expression);
            storage = join(inferExpression(binary.left), inferExpression(binary.right));
            break;
        }
        case SLExpression::Kind::TERNARY: {
            const SLTernary & ternary = static_cast<const SLTernary &>(*expression);
            storage = join(inferExpression(ternary.condition),
                join(inferExpression(ternary.whenTrue), inferExpression(ternary.whenFalse)));
            break;
        }
        case SLExpression::Kind::CAST:
            // the space is a name rather than an operand, and a name is uniform
            storage = inferExpression(static_cast<const SLCast &>(*expression).operand);
            break;
        case SLExpression::Kind::TUPLE: {
            const SLTuple & tuple = static_cast<const SLTuple &>(*expression);
            for (const SLExpressionPtr & element : tuple.elements) {
                storage = join(storage, inferExpression(element));
            }
            break;
        }
        case SLExpression::Kind::CALL:
            storage = inferCall(expression);
            break;
        case SLExpression::Kind::INDEX:
            break;
    }
    expression->storage = storage;
    return storage;
}

SLStorage SLCompiler::inferCall(const SLExpressionPtr & expression) {
    const SLCall & call = static_cast<const SLCall &>(*expression);
    SLStorage storage = SLStorage::UNIFORM;
    std::vector<SLStorage> arguments;
    arguments.reserve(call.arguments.size());
    for (const SLExpressionPtr & argument : call.arguments) {
        arguments.push_back(inferExpression(argument));
        storage = join(storage, arguments.back());
    }
    if (call.function < 0) {
        // a built-in that reads the shading point is varying however uniform its arguments
        // are: ambient() takes none and answers differently at every point on a grid
        const bool varying = call.signature >= 0 &&
            builtins()[static_cast<std::size_t>(call.signature)].varying;
        return varying ? SLStorage::VARYING : storage;
    }
    const std::size_t index = static_cast<std::size_t>(call.function);
    const SLFunction & function = shader_->functions[index];
    // a formal takes the storage of every argument any call site passes it, which is what
    // makes a function called once with a varying value varying everywhere
    for (std::size_t i = 0; i < arguments.size() && i < function.parameters.size(); i++) {
        if (arguments[i] == SLStorage::VARYING) {
            spread(function.parameters[i].symbol, call.arguments[i]);
        }
    }
    return join(storage, results_[index]);
}

};  // namespace v3d::render::offline
