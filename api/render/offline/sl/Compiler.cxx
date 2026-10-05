/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Compiler.h"

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
#include <api/render/offline/sl/syntax/Index.h>
#include <api/render/offline/sl/syntax/Jump.h>
#include <api/render/offline/sl/syntax/Lighting.h>
#include <api/render/offline/sl/syntax/Parameter.h>
#include <api/render/offline/sl/syntax/Shader.h>
#include <api/render/offline/sl/syntax/Statement.h>
#include <api/render/offline/sl/syntax/Ternary.h>
#include <api/render/offline/sl/syntax/Tuple.h>
#include <api/render/offline/sl/syntax/Unary.h>
#include <api/render/offline/sl/syntax/Variable.h>
#include <api/render/offline/sl/syntax/Walk.h>
#include <api/render/offline/sl/syntax/While.h>

#include <algorithm>
#include <string>
#include <vector>

#include "Builtins.h"
#include "Inference.h"
#include "Symbol.h"
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
 * shader mentioning `Ci` is told that `Ci` belongs to a surface and an imager, rather than
 * that it is undeclared.
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
    // an imager writes alpha as well as reading it: a pixel it paints counts as covered,
    // and the "background" imager writes alpha to record that
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

const char* construct(syntax::Lighting::Construct which) {
    switch (which) {
        case syntax::Lighting::Construct::ILLUMINANCE:
            return "illuminance";
        case syntax::Lighting::Construct::ILLUMINATE:
            return "illuminate";
        case syntax::Lighting::Construct::SOLAR:
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
            // a float replicates into a direction, so "normalize(0)" is accepted
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

bool defines(const std::vector<syntax::Function> & functions, const std::string & name) {
    return std::ranges::any_of(functions,
        [&name](const syntax::Function & function) { return function.name == name; });
}

void gather(const syntax::StatementPtr & statement, std::vector<std::string>* called);

/**
 * Every name an expression calls, added once. adopt() collects these from the tree before
 * any name has been resolved.
 **/
void gather(const syntax::ExpressionPtr & expression, std::vector<std::string>* called) {
    if (!expression) {
        return;
    }
    if (expression->kind == syntax::Expression::Kind::CALL) {
        const syntax::Call & call = static_cast<const syntax::Call &>(*expression);
        if (std::ranges::find(*called, call.name) == called->end()) {
            called->push_back(call.name);
        }
    }
    syntax::forEachChild(*expression, [called](const syntax::ExpressionPtr & child) { gather(child, called); });
}

void gather(const syntax::StatementPtr & statement, std::vector<std::string>* called) {
    if (!statement) {
        return;
    }
    syntax::forEachChild(*statement,
        [called](const syntax::ExpressionPtr & child) { gather(child, called); },
        [called](const syntax::StatementPtr & child) { gather(child, called); });
}

};  // namespace

Compiler::Compiler(const syntax::ShaderPtr & shader) : shader_(shader) {
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
    // or with another parameter is an error
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
    for (syntax::Parameter & parameter : shader_->parameters) {
        if (parameter.type == Type::VOID) {
            throw fail("a parameter cannot be void", parameter.line, parameter.column);
        }
        // a parameter is uniform unless declared varying: a scene binds one value for the
        // whole primitive unless the declaration lets the renderer vary it
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
        calls_.assign(shader_->functions.size(), std::vector<int>());
        checkFunctions();
        checkBlock(shader_->body);
        checkCallGraph();
        const std::string violation = Inference(shader_, &symbols_).run();
        if (!violation.empty() && error_.empty()) {
            error_ = violation;
            return false;
        }
    } catch (const Failure &) {
        return false;
    }
    return error_.empty();
}

void Compiler::adopt() {
    // a call to diffuse or specular names a function written in the language. It is added to
    // the shader's own functions so that nothing after this pass sees two kinds of function
    std::vector<std::string> called;
    gather(boost::static_pointer_cast<syntax::Statement>(shader_->body), &called);
    for (const syntax::Function & function : shader_->functions) {
        gather(boost::static_pointer_cast<syntax::Statement>(function.body), &called);
    }
    if (called.empty()) {
        return;
    }
    const std::vector<syntax::Function> library = sources();
    for (std::size_t i = 0; i < called.size(); i++) {
        if (defines(shader_->functions, called[i])) {
            // the shader's own takes precedence, so a scene can override one of these
            continue;
        }
        for (const syntax::Function & candidate : library) {
            if (candidate.name != called[i]) {
                continue;
            }
            shader_->functions.push_back(candidate);
            // add whatever it calls in turn, such as specularbrdf from specular
            gather(boost::static_pointer_cast<syntax::Statement>(candidate.body), &called);
            break;
        }
    }
}

void Compiler::checkFunctions() {
    for (std::size_t i = 0; i < shader_->functions.size(); i++) {
        syntax::Function & function = shader_->functions[i];
        const std::size_t mark = scope_.size();
        inside_ = static_cast<int>(i);
        for (syntax::Parameter & formal : function.parameters) {
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
    // depth first over the call graph, colouring as it goes
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
                        const syntax::Function & function = shader_->functions[next];
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

void Compiler::checkBlock(const syntax::BlockPtr & block) {
    if (!block) {
        return;
    }
    const std::size_t mark = scope_.size();
    for (const syntax::StatementPtr & statement : block->statements) {
        checkStatement(statement);
    }
    // a local goes out of scope with its block, and its symbol stays: the machine allocates
    // against the symbol, and two locals of the same name in sibling blocks are two symbols
    scope_.resize(mark);
}

void Compiler::checkStatement(const syntax::StatementPtr & statement) {
    if (!statement) {
        return;
    }
    switch (statement->kind) {
        case syntax::Statement::Kind::BLOCK:
            checkBlock(boost::static_pointer_cast<syntax::Block>(statement));
            return;
        case syntax::Statement::Kind::DECLARATION:
            checkDeclaration(statement);
            return;
        case syntax::Statement::Kind::ASSIGNMENT:
            checkAssignment(statement);
            return;
        case syntax::Statement::Kind::CONDITIONAL: {
            const syntax::Conditional & conditional = static_cast<const syntax::Conditional &>(*statement);
            checkCondition(conditional.condition, "if");
            checkStatement(conditional.whenTrue);
            checkStatement(conditional.whenFalse);
            return;
        }
        case syntax::Statement::Kind::WHILE: {
            const syntax::While & loop = static_cast<const syntax::While &>(*statement);
            checkCondition(loop.condition, "while");
            checkStatement(loop.body);
            return;
        }
        case syntax::Statement::Kind::FOR: {
            const syntax::For & loop = static_cast<const syntax::For &>(*statement);
            checkStatement(loop.initialiser);
            if (loop.condition) {
                checkCondition(loop.condition, "for");
            }
            checkStatement(loop.step);
            checkStatement(loop.body);
            return;
        }
        case syntax::Statement::Kind::JUMP:
            checkJump(statement);
            return;
        case syntax::Statement::Kind::EXPRESSION:
            checkExpression(static_cast<const syntax::ExpressionStatement &>(*statement).expression);
            return;
        case syntax::Statement::Kind::LIGHTING:
            checkLighting(statement);
            return;
    }
}

void Compiler::checkCondition(const syntax::ExpressionPtr & condition, const char* construct) {
    const Type type = checkExpression(condition);
    if (!coercible(type, Type::FLOAT)) {
        throw fail(std::string("the condition of a '") + construct + "' is " + name(type) +
            ", which is not a number", condition->line, condition->column);
    }
}

void Compiler::checkJump(const syntax::StatementPtr & statement) {
    const syntax::Jump & jump = static_cast<const syntax::Jump &>(*statement);
    if (!jump.value) {
        return;
    }
    if (inside_ < 0) {
        throw fail("a shader returns no value", statement->line, statement->column);
    }
    const syntax::Function & function = shader_->functions[static_cast<std::size_t>(inside_)];
    const Type given = checkExpression(jump.value);
    if (!coercible(given, function.type)) {
        throw fail("'" + function.name + "' returns " + name(function.type) + ", not " + name(given),
            statement->line, statement->column);
    }
}

void Compiler::checkDeclaration(const syntax::StatementPtr & statement) {
    syntax::Declaration & declaration = static_cast<syntax::Declaration &>(*statement);
    if (declaration.type == Type::VOID) {
        throw fail("a variable cannot be void", statement->line, statement->column);
    }
    for (syntax::Declarator & declarator : declaration.declarators) {
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

void Compiler::checkAssignment(const syntax::StatementPtr & statement) {
    const syntax::Assignment & assignment = static_cast<const syntax::Assignment &>(*statement);
    if (assignment.target->kind != syntax::Expression::Kind::VARIABLE) {
        throw fail("only a variable can be assigned to",
            assignment.target->line, assignment.target->column);
    }
    const Type target = checkExpression(assignment.target);
    const syntax::Variable & variable = static_cast<const syntax::Variable &>(*assignment.target);
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

void Compiler::checkLighting(const syntax::StatementPtr & statement) {
    syntax::Lighting & lighting = static_cast<syntax::Lighting &>(*statement);
    const bool surface = shader_->type == ShaderType::SURFACE;
    if (lighting.construct == syntax::Lighting::Construct::ILLUMINANCE) {
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
    for (const syntax::ExpressionPtr & argument : lighting.arguments) {
        checkExpression(argument);
    }
    depth_++;
    checkStatement(lighting.body);
    depth_--;
}

Type Compiler::checkExpression(const syntax::ExpressionPtr & expression) {
    if (!expression) {
        return Type::VOID;
    }
    switch (expression->kind) {
        case syntax::Expression::Kind::NUMBER:
            expression->type = Type::FLOAT;
            return expression->type;
        case syntax::Expression::Kind::STRING:
            expression->type = Type::STRING;
            return expression->type;
        case syntax::Expression::Kind::VARIABLE:
            return checkVariable(expression);
        case syntax::Expression::Kind::CALL:
            return checkCall(expression);
        case syntax::Expression::Kind::UNARY:
            return checkUnary(expression);
        case syntax::Expression::Kind::BINARY:
            return checkBinary(expression);
        case syntax::Expression::Kind::TERNARY:
            return checkTernary(expression);
        case syntax::Expression::Kind::CAST:
            return checkCast(expression);
        case syntax::Expression::Kind::TUPLE:
            // a parenthesised list is a literal for whatever a cast says it is, and means
            // nothing on its own - checkCast is the only place that reads one
            throw fail("a parenthesised list of values needs a type in front of it",
                expression->line, expression->column);
        case syntax::Expression::Kind::INDEX:
            throw fail("an array is not supported", expression->line, expression->column);
    }
    return Type::VOID;
}

Type Compiler::checkVariable(const syntax::ExpressionPtr & expression) {
    syntax::Variable & variable = static_cast<syntax::Variable &>(*expression);
    variable.symbol = lookup(variable.name);
    if (variable.symbol < 0) {
        // report a global of another shader type as that, rather than as an undeclared name
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

Type Compiler::checkUnary(const syntax::ExpressionPtr & expression) {
    const syntax::Unary & unary = static_cast<const syntax::Unary &>(*expression);
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

Type Compiler::checkBinary(const syntax::ExpressionPtr & expression) {
    const syntax::Binary & binary = static_cast<const syntax::Binary &>(*expression);
    const Type left = checkExpression(binary.left);
    const Type right = checkExpression(binary.right);
    const std::string & op = binary.op;

    if (op == "." || op == "^") {
        // the two that read as something else: a dot product and a cross product, over
        // positions and directions rather than over any three-component type
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

Type Compiler::checkTernary(const syntax::ExpressionPtr & expression) {
    const syntax::Ternary & ternary = static_cast<const syntax::Ternary &>(*expression);
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

Type Compiler::checkCast(const syntax::ExpressionPtr & expression) {
    const syntax::Cast & cast = static_cast<const syntax::Cast &>(*expression);
    if (cast.type == Type::VOID) {
        throw fail("nothing can be cast to void", expression->line, expression->column);
    }
    if (!cast.space.empty() && cast.type == Type::FLOAT) {
        throw fail("a coordinate space means nothing to a float", expression->line, expression->column);
    }
    if (cast.operand->kind == syntax::Expression::Kind::TUPLE) {
        // a parenthesised list is a literal for the type in front of it: three floats are a
        // point or a colour and sixteen are a matrix
        const syntax::Tuple & tuple = static_cast<const syntax::Tuple &>(*cast.operand);
        const unsigned int wanted = components(cast.type);
        if (tuple.elements.size() != wanted) {
            throw fail(std::string(name(cast.type)) + " is " + std::to_string(wanted) +
                " values, and " + std::to_string(tuple.elements.size()) + " were given",
                cast.operand->line, cast.operand->column);
        }
        for (const syntax::ExpressionPtr & element : tuple.elements) {
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
    if (cast.operand->kind == syntax::Expression::Kind::CALL) {
        wanted_ = cast.type;
    }
    const Type given = checkExpression(cast.operand);
    if (!coercible(given, cast.type)) {
        throw fail(std::string(name(given)) + " cannot be cast to " + name(cast.type),
            expression->line, expression->column);
    }
    expression->type = cast.type;
    return expression->type;
}

Type Compiler::checkCall(const syntax::ExpressionPtr & expression) {
    syntax::Call & call = static_cast<syntax::Call &>(*expression);
    const Type wanted = wanted_;
    wanted_ = Type::VOID;
    std::vector<Type> given;
    given.reserve(call.arguments.size());
    for (const syntax::ExpressionPtr & argument : call.arguments) {
        given.push_back(checkExpression(argument));
    }
    // a shader's own function takes precedence over a standard one of the same name
    const int function = checkShaderCall(call, given);
    if (function >= 0) {
        call.function = function;
        if (inside_ >= 0) {
            calls_[static_cast<std::size_t>(inside_)].push_back(function);
        }
        expression->type = shader_->functions[static_cast<std::size_t>(function)].type;
        return expression->type;
    }
    return checkBuiltinCall(call, given, wanted);
}

int Compiler::checkShaderCall(syntax::Call & call, const std::vector<Type> & given) {
    for (std::size_t i = 0; i < shader_->functions.size(); i++) {
        const syntax::Function & function = shader_->functions[i];
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

Type Compiler::checkBuiltinCall(syntax::Call & call, const std::vector<Type> & given, Type wanted) {
    const std::vector<Signature> & table = builtins();
    bool named = false;
    std::size_t chosen = table.size();
    for (std::size_t index = 0; index < table.size(); index++) {
        const Signature & signature = table[index];
        if (signature.name != call.name) {
            continue;
        }
        named = true;
        if (!suits(signature, given)) {
            continue;
        }
        if (chosen == table.size()) {
            chosen = index;
        }
        if (signature.resultFrom < 0 && signature.result == wanted) {
            chosen = index;
            break;
        }
    }
    if (!named) {
        throw fail("'" + call.name + "' is not a function", call.line, call.column);
    }
    if (chosen == table.size()) {
        throw fail("'" + call.name + "' cannot be called with those arguments", call.line, call.column);
    }
    const Signature & signature = table[chosen];
    for (std::size_t argument = 0; argument < given.size(); argument++) {
        const bool output = signature.outputs >= 0 && argument >= static_cast<std::size_t>(signature.outputs);
        const bool updated = signature.updates >= 0 && argument == static_cast<std::size_t>(signature.updates);
        if (!output && !updated) {
            continue;
        }
        const syntax::ExpressionPtr & written = call.arguments[argument];
        if (written->kind != syntax::Expression::Kind::VARIABLE) {
            throw fail("argument " + std::to_string(argument + 1) + " of '" + call.name +
                "' is written, so it has to be a variable", written->line, written->column);
        }
        const syntax::Variable & variable = static_cast<const syntax::Variable &>(*written);
        if (!symbols_[static_cast<std::size_t>(variable.symbol)].writable) {
            throw fail("argument " + std::to_string(argument + 1) + " of '" + call.name +
                "' is written, and '" + variable.name + "' cannot be assigned", written->line, written->column);
        }
    }
    call.signature = static_cast<int>(chosen);
    call.type = signature.resultFrom >= 0 ?
        given[static_cast<std::size_t>(signature.resultFrom)] : signature.result;
    return call.type;
}

};  // namespace v3d::render::offline::sl
