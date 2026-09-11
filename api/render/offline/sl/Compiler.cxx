/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Compiler.h"

#include <algorithm>
#include <string>
#include <vector>

#include "Builtins.h"
#include "Types.h"

namespace v3d::render::offline::sl {

namespace {

typedef Signature::Argument Argument;

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
    Type type;
    Storage storage;
    unsigned int shaders;
    unsigned int writable;
    /**
     * Only meaningful inside a lighting construct. A surface shader's L and Cl are set by
     * the light whose shader illuminance is running, and mean nothing outside that body.
     **/
    bool lighting;
};

const Global GLOBALS[] = {
    { "P", Type::POINT, Storage::VARYING, SURFACE | LIGHT | IMAGER, 0, false },
    { "N", Type::NORMAL, Storage::VARYING, SURFACE, 0, false },
    { "Ng", Type::NORMAL, Storage::VARYING, SURFACE, 0, false },
    { "I", Type::VECTOR, Storage::VARYING, SURFACE, 0, false },
    { "E", Type::POINT, Storage::UNIFORM, SURFACE, 0, false },
    { "Cs", Type::COLOR, Storage::VARYING, SURFACE, 0, false },
    { "Os", Type::COLOR, Storage::VARYING, SURFACE, 0, false },
    { "s", Type::FLOAT, Storage::VARYING, SURFACE, 0, false },
    { "t", Type::FLOAT, Storage::VARYING, SURFACE, 0, false },
    { "u", Type::FLOAT, Storage::VARYING, SURFACE, 0, false },
    { "v", Type::FLOAT, Storage::VARYING, SURFACE, 0, false },
    { "du", Type::FLOAT, Storage::VARYING, SURFACE, 0, false },
    { "dv", Type::FLOAT, Storage::VARYING, SURFACE, 0, false },
    { "Ci", Type::COLOR, Storage::VARYING, SURFACE | IMAGER, SURFACE | IMAGER, false },
    { "Oi", Type::COLOR, Storage::VARYING, SURFACE | IMAGER, SURFACE | IMAGER, false },
    { "Ps", Type::POINT, Storage::VARYING, LIGHT, 0, false },
    // an imager writes alpha as well as reading it: a pixel it has painted is no longer
    // one that nothing was drawn into, and "background" says so
    { "alpha", Type::FLOAT, Storage::VARYING, IMAGER, IMAGER, false },
    // a light writes these; a surface reads them, and only inside an illuminance body
    { "L", Type::VECTOR, Storage::VARYING, SURFACE | LIGHT, LIGHT, true },
    { "Cl", Type::COLOR, Storage::VARYING, SURFACE | LIGHT, LIGHT, true }
};

unsigned int bit(ShaderType type) {
    switch (type) {
        case ShaderType::SURFACE:
            return SURFACE;
        case ShaderType::LIGHT:
            return LIGHT;
        case ShaderType::IMAGER:
            return IMAGER;
        case ShaderType::DISPLACEMENT:
        case ShaderType::VOLUME:
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

const char* construct(Lighting::Construct which) {
    switch (which) {
        case Lighting::Construct::ILLUMINANCE:
            return "illuminance";
        case Lighting::Construct::ILLUMINATE:
            return "illuminate";
        case Lighting::Construct::SOLAR:
            return "solar";
    }
    return "";
}

std::string article(ShaderType type) {
    return type == ShaderType::IMAGER ? "an " + std::string(name(type)) : "a " + std::string(name(type));
}

bool accepts(Argument wanted, Type given) {
    switch (wanted) {
        case Argument::ANY:
            return given != Type::VOID;
        case Argument::NUMBER:
            return given == Type::FLOAT || given == Type::COLOR || pointlike(given);
        case Argument::POINTLIKE:
            // a float replicates into a direction, which is what "normalize(0)" leans on
            return pointlike(given) || given == Type::FLOAT;
        case Argument::FLOAT:
            return coercible(given, Type::FLOAT);
        case Argument::POINT:
            return coercible(given, Type::POINT);
        case Argument::VECTOR:
            return coercible(given, Type::VECTOR);
        case Argument::NORMAL:
            return coercible(given, Type::NORMAL);
        case Argument::COLOR:
            return coercible(given, Type::COLOR);
        case Argument::MATRIX:
            return coercible(given, Type::MATRIX);
        case Argument::STRING:
            return given == Type::STRING;
    }
    return false;
}

/**
 * Whether one way of calling a standard library function takes these argument types.
 **/
bool suits(const Signature & signature, const std::vector<Type> & given) {
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

bool defines(const std::vector<Function> & functions, const std::string & name) {
    return std::ranges::any_of(functions,
        [&name](const Function & function) { return function.name == name; });
}

void gather(const StatementPtr & statement, std::vector<std::string>* called);

/**
 * Every name an expression calls, added once. What adopt() walks the tree for, before any
 * name has been resolved to anything.
 **/
void gather(const ExpressionPtr & expression, std::vector<std::string>* called) {
    if (!expression) {
        return;
    }
    switch (expression->kind) {
        case Expression::Kind::CALL: {
            const Call & call = static_cast<const Call &>(*expression);
            if (std::ranges::find(*called, call.name) == called->end()) {
                called->push_back(call.name);
            }
            for (const ExpressionPtr & argument : call.arguments) {
                gather(argument, called);
            }
            return;
        }
        case Expression::Kind::UNARY:
            gather(static_cast<const Unary &>(*expression).operand, called);
            return;
        case Expression::Kind::BINARY: {
            const Binary & binary = static_cast<const Binary &>(*expression);
            gather(binary.left, called);
            gather(binary.right, called);
            return;
        }
        case Expression::Kind::TERNARY: {
            const Ternary & ternary = static_cast<const Ternary &>(*expression);
            gather(ternary.condition, called);
            gather(ternary.whenTrue, called);
            gather(ternary.whenFalse, called);
            return;
        }
        case Expression::Kind::CAST:
            gather(static_cast<const Cast &>(*expression).operand, called);
            return;
        case Expression::Kind::TUPLE:
            for (const ExpressionPtr & element : static_cast<const Tuple &>(*expression).elements) {
                gather(element, called);
            }
            return;
        case Expression::Kind::INDEX: {
            const Index & index = static_cast<const Index &>(*expression);
            gather(index.array, called);
            gather(index.index, called);
            return;
        }
        case Expression::Kind::NUMBER:
        case Expression::Kind::STRING:
        case Expression::Kind::VARIABLE:
            return;
    }
}

void gather(const StatementPtr & statement, std::vector<std::string>* called) {
    if (!statement) {
        return;
    }
    switch (statement->kind) {
        case Statement::Kind::BLOCK:
            for (const StatementPtr & inner : static_cast<const Block &>(*statement).statements) {
                gather(inner, called);
            }
            return;
        case Statement::Kind::DECLARATION:
            for (const Declarator & declarator : static_cast<const Declaration &>(*statement).declarators) {
                gather(declarator.initialiser, called);
            }
            return;
        case Statement::Kind::ASSIGNMENT: {
            const Assignment & assignment = static_cast<const Assignment &>(*statement);
            gather(assignment.target, called);
            gather(assignment.value, called);
            return;
        }
        case Statement::Kind::CONDITIONAL: {
            const Conditional & conditional = static_cast<const Conditional &>(*statement);
            gather(conditional.condition, called);
            gather(conditional.whenTrue, called);
            gather(conditional.whenFalse, called);
            return;
        }
        case Statement::Kind::WHILE: {
            const While & loop = static_cast<const While &>(*statement);
            gather(loop.condition, called);
            gather(loop.body, called);
            return;
        }
        case Statement::Kind::FOR: {
            const For & loop = static_cast<const For &>(*statement);
            gather(loop.initialiser, called);
            gather(loop.condition, called);
            gather(loop.step, called);
            gather(loop.body, called);
            return;
        }
        case Statement::Kind::JUMP:
            gather(static_cast<const Jump &>(*statement).value, called);
            return;
        case Statement::Kind::EXPRESSION:
            gather(static_cast<const ExpressionStatement &>(*statement).expression, called);
            return;
        case Statement::Kind::LIGHTING: {
            const Lighting & lighting = static_cast<const Lighting &>(*statement);
            for (const ExpressionPtr & argument : lighting.arguments) {
                gather(argument, called);
            }
            gather(lighting.body, called);
            return;
        }
    }
}

Storage join(Storage left, Storage right) {
    return left == Storage::VARYING || right == Storage::VARYING ?
        Storage::VARYING : Storage::UNIFORM;
}

};  // namespace

Compiler::Compiler(const ShaderPtr & shader) : shader_(shader) {
}

const std::string & Compiler::error() const {
    return error_;
}

const std::vector<Symbol> & Compiler::symbols() const {
    return symbols_;
}

Compiler::Failure Compiler::fail(const std::string & message, unsigned int line, unsigned int column) {
    if (error_.empty()) {
        error_ = message + " at line " + std::to_string(line) + ", column " + std::to_string(column);
    }
    return Failure();
}

int Compiler::declare(const std::string & name, Type type, Storage storage,
    Symbol::Role role, bool writable, unsigned int line, unsigned int column) {
    // a local shadows whatever is outside its block; a parameter that collides with a global
    // or with another parameter is a shader saying two things by one name
    if (role == Symbol::Role::PARAMETER) {
        const int existing = lookup(name);
        if (existing >= 0) {
            const bool global = symbols_[static_cast<std::size_t>(existing)].role == Symbol::Role::GLOBAL;
            throw fail("'" + name + "' is already " + (global ? "a shader global" : "a parameter"),
                line, column);
        }
    }
    Symbol symbol;
    symbol.name = name;
    symbol.type = type;
    symbol.storage = storage == Storage::UNSPECIFIED ? Storage::UNIFORM : storage;
    symbol.declared = storage != Storage::UNSPECIFIED;
    symbol.role = role;
    symbol.writable = writable;
    symbols_.push_back(symbol);

    Binding binding;
    binding.name = name;
    binding.symbol = static_cast<int>(symbols_.size()) - 1;
    scope_.push_back(binding);
    return binding.symbol;
}

int Compiler::lookup(const std::string & name) const {
    // innermost first, so an inner declaration shadows an outer one
    for (std::size_t i = scope_.size(); i > 0; i--) {
        if (scope_[i - 1].name == name) {
            return scope_[i - 1].symbol;
        }
    }
    return -1;
}

void Compiler::declareGlobals() {
    const unsigned int mine = bit(shader_->type);
    for (const Global & global : GLOBALS) {
        if ((global.shaders & mine) == 0) {
            continue;
        }
        const int index = declare(global.name, global.type, global.storage, Symbol::Role::GLOBAL,
            (global.writable & mine) != 0, shader_->line, shader_->column);
        if (global.lighting && (global.writable & mine) == 0) {
            lighting_.push_back(index);
        }
    }
}

void Compiler::declareParameters() {
    for (Parameter & parameter : shader_->parameters) {
        if (parameter.type == Type::VOID) {
            throw fail("a parameter cannot be void", parameter.line, parameter.column);
        }
        // a parameter is uniform unless it says otherwise: a scene binds one value for the
        // whole primitive, and only a declaration can say the renderer will vary it
        const Type given = checkExpression(parameter.defaultValue);
        if (!coercible(given, parameter.type)) {
            throw fail(std::string("the default for '") + parameter.name + "' is " + name(given) +
                ", which is not " + name(parameter.type), parameter.line, parameter.column);
        }
        parameter.symbol = declare(parameter.name, parameter.type, parameter.storage,
            Symbol::Role::PARAMETER, true, parameter.line, parameter.column);
        symbols_[parameter.symbol].output = parameter.output;
    }
}

bool Compiler::compile() {
    try {
        if (!shader_->supported()) {
            throw fail(std::string("a ") + name(shader_->type) + " shader is not supported",
                shader_->line, shader_->column);
        }
        declareGlobals();
        declareParameters();
        adopt();
        results_.assign(shader_->functions.size(), Storage::UNIFORM);
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

void Compiler::adopt() {
    // a call to diffuse or specular names a function written in the language, which the
    // shader takes on as its own so that nothing after this pass sees two kinds of function
    std::vector<std::string> called;
    gather(boost::static_pointer_cast<Statement>(shader_->body), &called);
    for (const Function & function : shader_->functions) {
        gather(boost::static_pointer_cast<Statement>(function.body), &called);
    }
    if (called.empty()) {
        return;
    }
    const std::vector<Function> library = sources();
    for (std::size_t i = 0; i < called.size(); i++) {
        if (defines(shader_->functions, called[i])) {
            // the shader's own wins, which is how a scene overrides one of these
            continue;
        }
        for (const Function & candidate : library) {
            if (candidate.name != called[i]) {
                continue;
            }
            shader_->functions.push_back(candidate);
            // and whatever it calls in turn, which is how specular reaches specularbrdf
            gather(boost::static_pointer_cast<Statement>(candidate.body), &called);
            break;
        }
    }
}

void Compiler::checkFunctions() {
    for (std::size_t i = 0; i < shader_->functions.size(); i++) {
        Function & function = shader_->functions[i];
        const std::size_t mark = scope_.size();
        inside_ = static_cast<int>(i);
        for (Parameter & formal : function.parameters) {
            if (formal.type == Type::VOID) {
                throw fail("a parameter cannot be void", formal.line, formal.column);
            }
            formal.symbol = declare(formal.name, formal.type, formal.storage,
                Symbol::Role::LOCAL, true, formal.line, formal.column);
        }
        checkBlock(function.body);
        scope_.resize(mark);
    }
    inside_ = -1;
}

void Compiler::checkCallGraph() {
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
                        const Function & function = shader_->functions[next];
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

void Compiler::checkBlock(const BlockPtr & block) {
    if (!block) {
        return;
    }
    const std::size_t mark = scope_.size();
    for (const StatementPtr & statement : block->statements) {
        checkStatement(statement);
    }
    // a local goes out of scope with its block, and its symbol stays: the machine allocates
    // against the symbol, and two locals of the same name in sibling blocks are two of them
    scope_.resize(mark);
}

void Compiler::checkStatement(const StatementPtr & statement) {
    if (!statement) {
        return;
    }
    switch (statement->kind) {
        case Statement::Kind::BLOCK:
            checkBlock(boost::static_pointer_cast<Block>(statement));
            return;
        case Statement::Kind::DECLARATION:
            checkDeclaration(statement);
            return;
        case Statement::Kind::ASSIGNMENT:
            checkAssignment(statement);
            return;
        case Statement::Kind::CONDITIONAL: {
            const Conditional & conditional = static_cast<const Conditional &>(*statement);
            checkCondition(conditional.condition, "if");
            checkStatement(conditional.whenTrue);
            checkStatement(conditional.whenFalse);
            return;
        }
        case Statement::Kind::WHILE: {
            const While & loop = static_cast<const While &>(*statement);
            checkCondition(loop.condition, "while");
            checkStatement(loop.body);
            return;
        }
        case Statement::Kind::FOR: {
            const For & loop = static_cast<const For &>(*statement);
            checkStatement(loop.initialiser);
            if (loop.condition) {
                checkCondition(loop.condition, "for");
            }
            checkStatement(loop.step);
            checkStatement(loop.body);
            return;
        }
        case Statement::Kind::JUMP:
            checkJump(statement);
            return;
        case Statement::Kind::EXPRESSION:
            checkExpression(static_cast<const ExpressionStatement &>(*statement).expression);
            return;
        case Statement::Kind::LIGHTING:
            checkLighting(statement);
            return;
    }
}

void Compiler::checkCondition(const ExpressionPtr & condition, const char* construct) {
    const Type type = checkExpression(condition);
    if (!coercible(type, Type::FLOAT)) {
        throw fail(std::string("the condition of a '") + construct + "' is " + name(type) +
            ", which is not a number", condition->line, condition->column);
    }
}

void Compiler::checkJump(const StatementPtr & statement) {
    const Jump & jump = static_cast<const Jump &>(*statement);
    if (!jump.value) {
        return;
    }
    if (inside_ < 0) {
        throw fail("a shader returns no value", statement->line, statement->column);
    }
    const Function & function = shader_->functions[static_cast<std::size_t>(inside_)];
    const Type given = checkExpression(jump.value);
    if (!coercible(given, function.type)) {
        throw fail("'" + function.name + "' returns " + name(function.type) + ", not " + name(given),
            statement->line, statement->column);
    }
}

void Compiler::checkDeclaration(const StatementPtr & statement) {
    Declaration & declaration = static_cast<Declaration &>(*statement);
    if (declaration.type == Type::VOID) {
        throw fail("a variable cannot be void", statement->line, statement->column);
    }
    for (Declarator & declarator : declaration.declarators) {
        if (declarator.initialiser) {
            // checked before the name is declared, so "float x = x" reads the outer x
            const Type given = checkExpression(declarator.initialiser);
            if (!coercible(given, declaration.type)) {
                throw fail("'" + declarator.name + "' is " + name(declaration.type) +
                    " and is given " + name(given), declarator.line, declarator.column);
            }
        }
        declarator.symbol = declare(declarator.name, declaration.type, declaration.storage,
            Symbol::Role::LOCAL, true, declarator.line, declarator.column);
    }
}

void Compiler::checkAssignment(const StatementPtr & statement) {
    const Assignment & assignment = static_cast<const Assignment &>(*statement);
    if (assignment.target->kind != Expression::Kind::VARIABLE) {
        throw fail("only a variable can be assigned to",
            assignment.target->line, assignment.target->column);
    }
    const Type target = checkExpression(assignment.target);
    const Variable & variable = static_cast<const Variable &>(*assignment.target);
    const Symbol & symbol = symbols_[static_cast<std::size_t>(variable.symbol)];
    if (!symbol.writable) {
        const std::string mine = article(shader_->type);
        if (symbol.role == Symbol::Role::GLOBAL) {
            throw fail("'" + variable.name + "' cannot be assigned in " + mine + " shader",
                assignment.target->line, assignment.target->column);
        }

        throw fail("'" + variable.name + "' cannot be assigned",
            assignment.target->line, assignment.target->column);
    }
    const Type given = checkExpression(assignment.value);
    // a compound form is the operator and then the assignment, so both have to work
    const Type combined = assignment.op == "=" ? given : arithmetic(target, given);
    if (combined == Type::VOID || !coercible(combined, target)) {
        throw fail("'" + variable.name + "' is " + name(target) + " and is given " + name(given),
            statement->line, statement->column);
    }
}

void Compiler::checkLighting(const StatementPtr & statement) {
    Lighting & lighting = static_cast<Lighting &>(*statement);
    const bool surface = shader_->type == ShaderType::SURFACE;
    if (lighting.construct == Lighting::Construct::ILLUMINANCE) {
        if (!surface) {
            throw fail("'illuminance' is only valid in a surface shader",
                statement->line, statement->column);
        }
        if (lighting.arguments.empty()) {
            throw fail("'illuminance' needs the point being shaded", statement->line, statement->column);
        }
    } else if (shader_->type != ShaderType::LIGHT) {
        throw fail(std::string("'") + construct(lighting.construct) + "' is only valid in a light shader",
            statement->line, statement->column);
    }
    for (const ExpressionPtr & argument : lighting.arguments) {
        checkExpression(argument);
    }
    depth_++;
    checkStatement(lighting.body);
    depth_--;
}

Type Compiler::checkExpression(const ExpressionPtr & expression) {
    if (!expression) {
        return Type::VOID;
    }
    switch (expression->kind) {
        case Expression::Kind::NUMBER:
            expression->type = Type::FLOAT;
            return expression->type;
        case Expression::Kind::STRING:
            expression->type = Type::STRING;
            return expression->type;
        case Expression::Kind::VARIABLE:
            return checkVariable(expression);
        case Expression::Kind::CALL:
            return checkCall(expression);
        case Expression::Kind::UNARY:
            return checkUnary(expression);
        case Expression::Kind::BINARY:
            return checkBinary(expression);
        case Expression::Kind::TERNARY:
            return checkTernary(expression);
        case Expression::Kind::CAST:
            return checkCast(expression);
        case Expression::Kind::TUPLE:
            // a parenthesised list is a literal for whatever a cast says it is, and means
            // nothing on its own - checkCast is the only place that reads one
            throw fail("a parenthesised list of values needs a type in front of it",
                expression->line, expression->column);
        case Expression::Kind::INDEX:
            throw fail("an array is not supported", expression->line, expression->column);
    }
    return Type::VOID;
}

Type Compiler::checkVariable(const ExpressionPtr & expression) {
    Variable & variable = static_cast<Variable &>(*expression);
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

Type Compiler::checkUnary(const ExpressionPtr & expression) {
    const Unary & unary = static_cast<const Unary &>(*expression);
    const Type operand = checkExpression(unary.operand);
    if (unary.op == "!") {
        if (!coercible(operand, Type::FLOAT)) {
            throw fail("'!' takes a number, not " + std::string(name(operand)),
                expression->line, expression->column);
        }
        expression->type = Type::FLOAT;
        return expression->type;
    }
    if (operand == Type::STRING || operand == Type::VOID) {
        throw fail("'-' takes a number, not " + std::string(name(operand)),
            expression->line, expression->column);
    }
    expression->type = operand;
    return expression->type;
}

Type Compiler::checkBinary(const ExpressionPtr & expression) {
    const Binary & binary = static_cast<const Binary &>(*expression);
    const Type left = checkExpression(binary.left);
    const Type right = checkExpression(binary.right);
    const std::string & op = binary.op;

    if (op == "." || op == "^") {
        // the two that read as something else: a dot product and a cross product, over
        // positions and directions rather than over anything with three of something
        if (!accepts(Argument::POINTLIKE, left) || !accepts(Argument::POINTLIKE, right)) {
            throw fail("'" + op + "' takes two positions or directions, not " +
                name(left) + " and " + name(right), expression->line, expression->column);
        }
        expression->type = op == "." ? Type::FLOAT : Type::VECTOR;
        return expression->type;
    }
    if (op == "&&" || op == "||") {
        if (!coercible(left, Type::FLOAT) || !coercible(right, Type::FLOAT)) {
            throw fail("'" + op + "' takes numbers, not " + name(left) + " and " + name(right),
                expression->line, expression->column);
        }
        expression->type = Type::FLOAT;
        return expression->type;
    }
    if (op == "<" || op == "<=" || op == ">" || op == ">=") {
        if (!coercible(left, Type::FLOAT) || !coercible(right, Type::FLOAT)) {
            throw fail("'" + op + "' compares numbers, not " + name(left) + " and " + name(right),
                expression->line, expression->column);
        }
        expression->type = Type::FLOAT;
        return expression->type;
    }
    if (op == "==" || op == "!=") {
        if (!coercible(left, right) && !coercible(right, left)) {
            throw fail("'" + op + "' cannot compare " + name(left) + " with " + name(right),
                expression->line, expression->column);
        }
        expression->type = Type::FLOAT;
        return expression->type;
    }
    const Type combined = arithmetic(left, right);
    if (combined == Type::VOID) {
        throw fail("'" + op + "' has no meaning between " + name(left) + " and " + name(right),
            expression->line, expression->column);
    }
    expression->type = combined;
    return expression->type;
}

Type Compiler::checkTernary(const ExpressionPtr & expression) {
    const Ternary & ternary = static_cast<const Ternary &>(*expression);
    checkCondition(ternary.condition, "?:");
    const Type whenTrue = checkExpression(ternary.whenTrue);
    const Type whenFalse = checkExpression(ternary.whenFalse);
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

Type Compiler::checkCast(const ExpressionPtr & expression) {
    const Cast & cast = static_cast<const Cast &>(*expression);
    if (cast.type == Type::VOID) {
        throw fail("nothing can be cast to void", expression->line, expression->column);
    }
    if (!cast.space.empty() && cast.type == Type::FLOAT) {
        throw fail("a coordinate space means nothing to a float", expression->line, expression->column);
    }
    if (cast.operand->kind == Expression::Kind::TUPLE) {
        // a parenthesised list is a literal for the type in front of it: three floats are a
        // point or a colour and sixteen are a matrix
        const Tuple & tuple = static_cast<const Tuple &>(*cast.operand);
        const unsigned int wanted = components(cast.type);
        if (tuple.elements.size() != wanted) {
            throw fail(std::string(name(cast.type)) + " is " + std::to_string(wanted) +
                " values, and " + std::to_string(tuple.elements.size()) + " were given",
                cast.operand->line, cast.operand->column);
        }
        for (const ExpressionPtr & element : tuple.elements) {
            const Type given = checkExpression(element);
            if (!coercible(given, Type::FLOAT)) {
                throw fail("a value of " + std::string(name(cast.type)) + " is a number, not " + name(given),
                    element->line, element->column);
            }
        }
        cast.operand->type = cast.type;
        expression->type = cast.type;
        return expression->type;
    }
    const Type given = checkExpression(cast.operand);
    if (!coercible(given, cast.type)) {
        throw fail(std::string(name(given)) + " cannot be cast to " + name(cast.type),
            expression->line, expression->column);
    }
    expression->type = cast.type;
    return expression->type;
}

Type Compiler::checkCall(const ExpressionPtr & expression) {
    Call & call = static_cast<Call &>(*expression);
    std::vector<Type> given;
    given.reserve(call.arguments.size());
    for (const ExpressionPtr & argument : call.arguments) {
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

int Compiler::checkShaderCall(Call & call, const std::vector<Type> & given) {
    for (std::size_t i = 0; i < shader_->functions.size(); i++) {
        const Function & function = shader_->functions[i];
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

Type Compiler::checkBuiltinCall(Call & call, const std::vector<Type> & given) {
    const std::vector<Signature> & table = builtins();
    bool named = false;
    for (std::size_t index = 0; index < table.size(); index++) {
        const Signature & signature = table[index];
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

bool Compiler::escapes(const StatementPtr & loop) const {
    return std::ranges::find(escaping_, loop.get()) != escaping_.end();
}

void Compiler::mark(const Statement* loop) {
    if (std::ranges::find(escaping_, loop) != escaping_.end()) {
        return;
    }
    escaping_.push_back(loop);
    changed_ = true;
}

void Compiler::spread(int symbol, const ExpressionPtr & from) {
    Symbol & entry = symbols_[static_cast<std::size_t>(symbol)];
    if (entry.storage == Storage::VARYING) {
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
    entry.storage = Storage::VARYING;
    changed_ = true;
}

void Compiler::infer() {
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

void Compiler::inferBlock(const BlockPtr & block, bool varyingContext) {
    if (!block) {
        return;
    }
    for (const StatementPtr & statement : block->statements) {
        inferStatement(statement, varyingContext);
    }
}

void Compiler::inferStatement(const StatementPtr & statement, bool varyingContext) {
    if (!statement) {
        return;
    }
    switch (statement->kind) {
        case Statement::Kind::BLOCK:
            inferBlock(boost::static_pointer_cast<Block>(statement), varyingContext);
            return;
        case Statement::Kind::DECLARATION:
            inferDeclaration(statement, varyingContext);
            return;
        case Statement::Kind::ASSIGNMENT:
            inferAssignment(statement, varyingContext);
            return;
        case Statement::Kind::CONDITIONAL: {
            const Conditional & conditional = static_cast<const Conditional &>(*statement);
            const bool varying = inferExpression(conditional.condition) == Storage::VARYING;
            inferStatement(conditional.whenTrue, varyingContext || varying);
            inferStatement(conditional.whenFalse, varyingContext || varying);
            return;
        }
        case Statement::Kind::WHILE: {
            const While & loop = static_cast<const While &>(*statement);
            const bool varying = inferExpression(loop.condition) == Storage::VARYING;
            enclosing_.push_back(statement.get());
            inferStatement(loop.body, varyingContext || varying || escapes(statement));
            enclosing_.pop_back();
            return;
        }
        case Statement::Kind::FOR: {
            const For & loop = static_cast<const For &>(*statement);
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
        case Statement::Kind::JUMP:
            inferJump(statement, varyingContext);
            return;
        case Statement::Kind::EXPRESSION:
            inferExpression(static_cast<const ExpressionStatement &>(*statement).expression);
            return;
        case Statement::Kind::LIGHTING: {
            const Lighting & lighting = static_cast<const Lighting &>(*statement);
            for (const ExpressionPtr & argument : lighting.arguments) {
                inferExpression(argument);
            }
            // the body runs once per light with L and Cl set per point, so everything it
            // writes is varying whatever reached it
            inferStatement(lighting.body, true);
            return;
        }
    }
}

void Compiler::inferDeclaration(const StatementPtr & statement, bool varyingContext) {
    const Declaration & declaration = static_cast<const Declaration &>(*statement);
    for (const Declarator & declarator : declaration.declarators) {
        const Storage value = declarator.initialiser ?
            inferExpression(declarator.initialiser) : Storage::UNIFORM;
        if (varyingContext || value == Storage::VARYING) {
            spread(declarator.symbol, declarator.initialiser);
        }
    }
}

void Compiler::inferAssignment(const StatementPtr & statement, bool varyingContext) {
    const Assignment & assignment = static_cast<const Assignment &>(*statement);
    const Storage value = inferExpression(assignment.value);
    inferExpression(assignment.target);
    const Variable & variable = static_cast<const Variable &>(*assignment.target);
    // different points take different arms, so anything written under a varying condition is
    // varying whatever was written to it
    if (varyingContext || value == Storage::VARYING) {
        spread(variable.symbol, assignment.value);
    }
}

void Compiler::inferJump(const StatementPtr & statement, bool varyingContext) {
    const Jump & jump = static_cast<const Jump &>(*statement);
    if (jump.where != Jump::Where::RETURN) {
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

Storage Compiler::inferExpression(const ExpressionPtr & expression) {
    if (!expression) {
        return Storage::UNIFORM;
    }
    Storage storage = Storage::UNIFORM;
    switch (expression->kind) {
        case Expression::Kind::NUMBER:
        case Expression::Kind::STRING:
            // a string names a coordinate space, a texture or a message, and there is no
            // per-point one to name: allowing one would make every transform a runtime
            // string lookup
            storage = Storage::UNIFORM;
            break;
        case Expression::Kind::VARIABLE:
            storage = symbols_[static_cast<std::size_t>(
                static_cast<const Variable &>(*expression).symbol)].storage;
            break;
        case Expression::Kind::UNARY:
            storage = inferExpression(static_cast<const Unary &>(*expression).operand);
            break;
        case Expression::Kind::BINARY: {
            const Binary & binary = static_cast<const Binary &>(*expression);
            storage = join(inferExpression(binary.left), inferExpression(binary.right));
            break;
        }
        case Expression::Kind::TERNARY: {
            const Ternary & ternary = static_cast<const Ternary &>(*expression);
            storage = join(inferExpression(ternary.condition),
                join(inferExpression(ternary.whenTrue), inferExpression(ternary.whenFalse)));
            break;
        }
        case Expression::Kind::CAST:
            // the space is a name rather than an operand, and a name is uniform
            storage = inferExpression(static_cast<const Cast &>(*expression).operand);
            break;
        case Expression::Kind::TUPLE: {
            const Tuple & tuple = static_cast<const Tuple &>(*expression);
            for (const ExpressionPtr & element : tuple.elements) {
                storage = join(storage, inferExpression(element));
            }
            break;
        }
        case Expression::Kind::CALL:
            storage = inferCall(expression);
            break;
        case Expression::Kind::INDEX:
            break;
    }
    expression->storage = storage;
    return storage;
}

Storage Compiler::inferCall(const ExpressionPtr & expression) {
    const Call & call = static_cast<const Call &>(*expression);
    Storage storage = Storage::UNIFORM;
    std::vector<Storage> arguments;
    arguments.reserve(call.arguments.size());
    for (const ExpressionPtr & argument : call.arguments) {
        arguments.push_back(inferExpression(argument));
        storage = join(storage, arguments.back());
    }
    if (call.function < 0) {
        // a built-in that reads the shading point is varying however uniform its arguments
        // are: ambient() takes none and answers differently at every point on a grid
        const bool varying = call.signature >= 0 &&
            builtins()[static_cast<std::size_t>(call.signature)].varying;
        return varying ? Storage::VARYING : storage;
    }
    const std::size_t index = static_cast<std::size_t>(call.function);
    const Function & function = shader_->functions[index];
    // a formal takes the storage of every argument any call site passes it, which is what
    // makes a function called once with a varying value varying everywhere
    for (std::size_t i = 0; i < arguments.size() && i < function.parameters.size(); i++) {
        if (arguments[i] == Storage::VARYING) {
            spread(function.parameters[i].symbol, call.arguments[i]);
        }
    }
    return join(storage, results_[index]);
}

};  // namespace v3d::render::offline::sl
