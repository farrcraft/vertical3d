/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Parser.h"

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

#include <istream>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::offline::sl {

namespace {

std::string position(const Token & token) {
    return " at line " + std::to_string(token.line()) + ", column " + std::to_string(token.column());
}

/**
 * What a token is, for a diagnostic. An END has no text to quote, and "end of source" is
 * what a reader needs to hear rather than an empty pair of quotes.
 **/
std::string describe(const Token & token) {
    switch (token.kind()) {
        case Token::Kind::END:
            return "end of source";
        case Token::Kind::NUMBER:
            return "the number " + std::to_string(token.value());
        case Token::Kind::STRING:
            return "the string \"" + token.text() + "\"";
        default:
            return "'" + token.text() + "'";
    }
}

bool shaderType(const Token & token, ShaderType* type) {
    if (token.kind() != Token::Kind::KEYWORD) {
        return false;
    }
    if (token.text() == "surface") {
        *type = ShaderType::SURFACE;
    } else if (token.text() == "light") {
        *type = ShaderType::LIGHT;
    } else if (token.text() == "displacement") {
        *type = ShaderType::DISPLACEMENT;
    } else if (token.text() == "volume") {
        *type = ShaderType::VOLUME;
    } else if (token.text() == "imager") {
        *type = ShaderType::IMAGER;
    } else {
        return false;
    }
    return true;
}

bool assignment(const Token & token) {
    if (token.kind() != Token::Kind::OPERATOR) {
        return false;
    }
    const std::string & text = token.text();
    return text == "=" || text == "+=" || text == "-=" || text == "*=" || text == "/=";
}

};  // namespace

Parser::Parser(std::istream & stream) : lexer_(stream) {
}

const std::string & Parser::error() const {
    return error_;
}

Parser::Failure Parser::fail(const std::string & message, const Token & token) {
    if (error_.empty()) {
        error_ = message + position(token);
    }
    return Failure();
}

const Token & Parser::peek() {
    const Token & token = lexer_.peek();
    if (!lexer_.error().empty()) {
        // the lexer's message already says where, so it stands as it is rather than being
        // wrapped in one about the END it handed back
        if (error_.empty()) {
            error_ = lexer_.error();
        }
        throw Failure();
    }
    return token;
}

Token Parser::next() {
    peek();
    return lexer_.next();
}

bool Parser::at(Token::Kind kind, const std::string & text) {
    const Token & token = peek();
    return token.kind() == kind && token.text() == text;
}

bool Parser::accept(Token::Kind kind, const std::string & text) {
    if (!at(kind, text)) {
        return false;
    }
    next();
    return true;
}

Token Parser::expect(Token::Kind kind, const std::string & text) {
    const Token & token = peek();
    if (token.kind() != kind || token.text() != text) {
        throw fail("expected '" + text + "' but found " + describe(token), token);
    }
    return next();
}

Token Parser::expectKind(Token::Kind kind, const char* what) {
    const Token & token = peek();
    if (token.kind() != kind) {
        throw fail(std::string("expected ") + what + " but found " + describe(token), token);
    }
    return next();
}

std::vector<syntax::ShaderPtr> Parser::parse() {
    std::vector<syntax::ShaderPtr> shaders;
    try {
        while (peek().kind() != Token::Kind::END) {
            shaders.push_back(parseShader());
        }
    } catch (const Failure &) {
        // a parse that fails yields no program at all: half a shader is worse than none,
        // because a renderer would run it
        return std::vector<syntax::ShaderPtr>();
    }
    return shaders;
}

syntax::ShaderPtr Parser::parseShader() {
    const Token opening = peek();
    ShaderType type = ShaderType::SURFACE;
    if (!shaderType(opening, &type)) {
        throw fail("expected a shader type but found " + describe(opening), opening);
    }
    syntax::ShaderPtr shader = boost::make_shared<syntax::Shader>();
    shader->type = type;
    shader->line = opening.line();
    shader->column = opening.column();
    next();

    shader->name = expectKind(Token::Kind::IDENTIFIER, "a shader name").text();
    expect(Token::Kind::PUNCTUATION, "(");
    if (!at(Token::Kind::PUNCTUATION, ")")) {
        parseParameters(&shader->parameters, true);
    }
    expect(Token::Kind::PUNCTUATION, ")");

    const Token brace = expect(Token::Kind::PUNCTUATION, "{");
    shader->body = boost::make_shared<syntax::Block>(brace.line(), brace.column());
    while (!at(Token::Kind::PUNCTUATION, "}")) {
        if (peek().kind() == Token::Kind::END) {
            throw fail("expected '}' but found end of source", peek());
        }
        // a storage class can only begin a declaration; a bare type keyword begins either a
        // declaration or a function, and the token after the name is what tells them apart
        const Storage storage = parseStorage();
        Type declared = Type::FLOAT;
        if (storage != Storage::UNSPECIFIED) {
            const Token token = peek();
            if (!parseType(&declared)) {
                throw fail("expected a type but found " + describe(token), token);
            }
            const Token name = expectKind(Token::Kind::IDENTIFIER, "a name");
            shader->body->statements.push_back(parseDeclaration(storage, declared, name));
            expect(Token::Kind::PUNCTUATION, ";");
            continue;
        }
        const Token typeToken = peek();
        if (!parseType(&declared)) {
            shader->body->statements.push_back(parseStatement());
            continue;
        }
        const Token name = expectKind(Token::Kind::IDENTIFIER, "a name");
        if (!at(Token::Kind::PUNCTUATION, "(")) {
            shader->body->statements.push_back(parseDeclaration(storage, declared, name));
            expect(Token::Kind::PUNCTUATION, ";");
            continue;
        }
        syntax::Function function;
        function.type = declared;
        function.name = name.text();
        function.line = typeToken.line();
        function.column = typeToken.column();
        expect(Token::Kind::PUNCTUATION, "(");
        if (!at(Token::Kind::PUNCTUATION, ")")) {
            parseParameters(&function.parameters, false);
        }
        expect(Token::Kind::PUNCTUATION, ")");
        function.body = parseBlock();
        shader->functions.push_back(function);
    }
    expect(Token::Kind::PUNCTUATION, "}");
    return shader;
}

void Parser::parseParameters(std::vector<syntax::Parameter>* parameters, bool defaults) {
    for (;;) {
        const Token opening = peek();
        syntax::Parameter parameter;
        parameter.line = opening.line();
        parameter.column = opening.column();

        // "output varying color Ci = 0" is how SL writes it, but neither order is worth
        // rejecting, and 'output' is an identifier rather than a keyword
        for (;;) {
            if (accept(Token::Kind::IDENTIFIER, "output")) {
                parameter.output = true;
                continue;
            }
            const Storage storage = parseStorage();
            if (storage != Storage::UNSPECIFIED) {
                parameter.storage = storage;
                continue;
            }
            break;
        }

        const Token typeToken = peek();
        if (!parseType(&parameter.type)) {
            throw fail("expected a parameter type but found " + describe(typeToken), typeToken);
        }
        parameter.name = expectKind(Token::Kind::IDENTIFIER, "a parameter name").text();
        if (defaults) {
            // SL has no uninitialised shader parameter: the default is what a scene that
            // does not mention the parameter gets, so a missing one is an error rather
            // than a zero
            expect(Token::Kind::OPERATOR, "=");
            parameter.defaultValue = parseExpression();
        }
        parameters->push_back(parameter);

        // a shader's parameters are separated by semicolons and a function's formals by
        // commas, and a file written the other way round is not worth refusing
        if (!accept(Token::Kind::PUNCTUATION, ";") && !accept(Token::Kind::PUNCTUATION, ",")) {
            return;
        }
        if (at(Token::Kind::PUNCTUATION, ")")) {
            return;
        }
    }
}

bool Parser::parseType(Type* type) {
    const Token & token = peek();
    if (token.kind() != Token::Kind::KEYWORD) {
        return false;
    }
    const std::string & text = token.text();
    if (text == "float") {
        *type = Type::FLOAT;
    } else if (text == "point") {
        *type = Type::POINT;
    } else if (text == "vector") {
        *type = Type::VECTOR;
    } else if (text == "normal") {
        *type = Type::NORMAL;
    } else if (text == "color") {
        *type = Type::COLOR;
    } else if (text == "matrix") {
        *type = Type::MATRIX;
    } else if (text == "string") {
        *type = Type::STRING;
    } else if (text == "void") {
        *type = Type::VOID;
    } else {
        return false;
    }
    next();
    return true;
}

Storage Parser::parseStorage() {
    if (accept(Token::Kind::KEYWORD, "uniform")) {
        return Storage::UNIFORM;
    }
    if (accept(Token::Kind::KEYWORD, "varying")) {
        return Storage::VARYING;
    }
    return Storage::UNSPECIFIED;
}

syntax::BlockPtr Parser::parseBlock() {
    const Token brace = expect(Token::Kind::PUNCTUATION, "{");
    syntax::BlockPtr block = boost::make_shared<syntax::Block>(brace.line(), brace.column());
    while (!at(Token::Kind::PUNCTUATION, "}")) {
        if (peek().kind() == Token::Kind::END) {
            throw fail("expected '}' but found end of source", peek());
        }
        block->statements.push_back(parseStatement());
    }
    expect(Token::Kind::PUNCTUATION, "}");
    return block;
}

syntax::StatementPtr Parser::parseStatement() {
    const Token token = peek();
    if (token.kind() == Token::Kind::PUNCTUATION && token.text() == "{") {
        return parseBlock();
    }
    if (accept(Token::Kind::PUNCTUATION, ";")) {
        // an empty statement is an empty block, which is what it does
        return boost::make_shared<syntax::Block>(token.line(), token.column());
    }
    if (token.kind() == Token::Kind::KEYWORD) {
        return parseKeywordStatement(token);
    }
    syntax::StatementPtr statement = parseSimpleStatement();
    expect(Token::Kind::PUNCTUATION, ";");
    return statement;
}

syntax::StatementPtr Parser::parseKeywordStatement(const Token & keyword) {
    const std::string & text = keyword.text();
    if (text == "if") {
        return parseConditional();
    }
    if (text == "while") {
        return parseWhile();
    }
    if (text == "for") {
        return parseFor();
    }
    if (text == "break" || text == "continue" || text == "return") {
        return parseJump();
    }
    if (text == "illuminance") {
        return parseLighting(syntax::Lighting::Construct::ILLUMINANCE);
    }
    if (text == "illuminate") {
        return parseLighting(syntax::Lighting::Construct::ILLUMINATE);
    }
    if (text == "solar") {
        return parseLighting(syntax::Lighting::Construct::SOLAR);
    }
    if (text == "else") {
        throw fail("'else' without a matching 'if'", keyword);
    }
    return parseLocalDeclaration();
}

syntax::StatementPtr Parser::parseConditional() {
    const Token keyword = expect(Token::Kind::KEYWORD, "if");
    expect(Token::Kind::PUNCTUATION, "(");
    syntax::ExpressionPtr condition = parseExpression();
    expect(Token::Kind::PUNCTUATION, ")");
    syntax::StatementPtr whenTrue = parseStatement();
    syntax::StatementPtr whenFalse;
    // an else binds to the nearest if, which is what taking it here rather than unwinding
    // to an outer one does
    if (accept(Token::Kind::KEYWORD, "else")) {
        whenFalse = parseStatement();
    }
    return boost::make_shared<syntax::Conditional>(condition, whenTrue, whenFalse,
        keyword.line(), keyword.column());
}

syntax::StatementPtr Parser::parseWhile() {
    const Token keyword = expect(Token::Kind::KEYWORD, "while");
    expect(Token::Kind::PUNCTUATION, "(");
    syntax::ExpressionPtr condition = parseExpression();
    expect(Token::Kind::PUNCTUATION, ")");
    return boost::make_shared<syntax::While>(condition, parseStatement(), keyword.line(), keyword.column());
}

syntax::StatementPtr Parser::parseFor() {
    const Token keyword = expect(Token::Kind::KEYWORD, "for");
    boost::shared_ptr<syntax::For> loop = boost::make_shared<syntax::For>(keyword.line(), keyword.column());
    expect(Token::Kind::PUNCTUATION, "(");
    // any of the three heads may be empty, which is what a null one on the node is
    if (!at(Token::Kind::PUNCTUATION, ";")) {
        loop->initialiser = parseSimpleStatement();
    }
    expect(Token::Kind::PUNCTUATION, ";");
    if (!at(Token::Kind::PUNCTUATION, ";")) {
        loop->condition = parseExpression();
    }
    expect(Token::Kind::PUNCTUATION, ";");
    if (!at(Token::Kind::PUNCTUATION, ")")) {
        loop->step = parseSimpleStatement();
    }
    expect(Token::Kind::PUNCTUATION, ")");
    loop->body = parseStatement();
    return loop;
}

syntax::StatementPtr Parser::parseJump() {
    const Token keyword = next();
    syntax::Jump::Where where = syntax::Jump::Where::RETURN;
    syntax::ExpressionPtr value;
    if (keyword.text() == "break") {
        where = syntax::Jump::Where::BREAK;
    } else if (keyword.text() == "continue") {
        where = syntax::Jump::Where::CONTINUE;
    } else if (!at(Token::Kind::PUNCTUATION, ";")) {
        value = parseExpression();
    }
    expect(Token::Kind::PUNCTUATION, ";");
    return boost::make_shared<syntax::Jump>(where, value, keyword.line(), keyword.column());
}

syntax::StatementPtr Parser::parseLocalDeclaration() {
    const Storage storage = parseStorage();
    Type type = Type::FLOAT;
    const Token typeToken = peek();
    if (!parseType(&type)) {
        throw fail("expected a type but found " + describe(typeToken), typeToken);
    }
    const Token name = expectKind(Token::Kind::IDENTIFIER, "a name");
    if (at(Token::Kind::PUNCTUATION, "(")) {
        throw fail("a function may only be defined at the top of a shader body", name);
    }
    syntax::StatementPtr declaration = parseDeclaration(storage, type, name);
    expect(Token::Kind::PUNCTUATION, ";");
    return declaration;
}

syntax::StatementPtr Parser::parseDeclaration(Storage storage, Type type, const Token & first) {
    boost::shared_ptr<syntax::Declaration> declaration =
        boost::make_shared<syntax::Declaration>(storage, type, first.line(), first.column());
    Token name = first;
    for (;;) {
        syntax::Declarator declarator;
        declarator.name = name.text();
        declarator.line = name.line();
        declarator.column = name.column();
        if (accept(Token::Kind::OPERATOR, "=")) {
            declarator.initialiser = parseExpression();
        }
        declaration->declarators.push_back(declarator);
        if (!accept(Token::Kind::PUNCTUATION, ",")) {
            return declaration;
        }
        name = expectKind(Token::Kind::IDENTIFIER, "a name");
    }
}

syntax::StatementPtr Parser::parseSimpleStatement() {
    const Token opening = peek();
    syntax::ExpressionPtr expression = parseExpression();
    if (!assignment(peek())) {
        return boost::make_shared<syntax::ExpressionStatement>(expression, opening.line(), opening.column());
    }
    const Token op = next();
    syntax::ExpressionPtr value = parseExpression();
    return boost::make_shared<syntax::Assignment>(op.text(), expression, value, opening.line(), opening.column());
}

syntax::StatementPtr Parser::parseLighting(syntax::Lighting::Construct construct) {
    const Token keyword = next();
    boost::shared_ptr<syntax::Lighting> lighting =
        boost::make_shared<syntax::Lighting>(construct, keyword.line(), keyword.column());
    expect(Token::Kind::PUNCTUATION, "(");
    if (!at(Token::Kind::PUNCTUATION, ")")) {
        lighting->arguments.push_back(parseExpression());
        while (accept(Token::Kind::PUNCTUATION, ",")) {
            lighting->arguments.push_back(parseExpression());
        }
    }
    expect(Token::Kind::PUNCTUATION, ")");
    lighting->body = parseStatement();
    return lighting;
}

syntax::ExpressionPtr Parser::parseExpression() {
    return parseTernary();
}

syntax::ExpressionPtr Parser::parseTernary() {
    syntax::ExpressionPtr condition = parseLogicalOr();
    const Token question = peek();
    if (!accept(Token::Kind::OPERATOR, "?")) {
        return condition;
    }
    // right associative, so the arms are ternaries themselves rather than one level down
    syntax::ExpressionPtr whenTrue = parseTernary();
    expect(Token::Kind::OPERATOR, ":");
    syntax::ExpressionPtr whenFalse = parseTernary();
    return boost::make_shared<syntax::Ternary>(condition, whenTrue, whenFalse, question.line(), question.column());
}

syntax::ExpressionPtr Parser::parseLogicalOr() {
    syntax::ExpressionPtr left = parseLogicalAnd();
    for (;;) {
        const Token op = peek();
        if (!accept(Token::Kind::OPERATOR, "||")) {
            return left;
        }
        left = boost::make_shared<syntax::Binary>(op.text(), left, parseLogicalAnd(), op.line(), op.column());
    }
}

syntax::ExpressionPtr Parser::parseLogicalAnd() {
    syntax::ExpressionPtr left = parseEquality();
    for (;;) {
        const Token op = peek();
        if (!accept(Token::Kind::OPERATOR, "&&")) {
            return left;
        }
        left = boost::make_shared<syntax::Binary>(op.text(), left, parseEquality(), op.line(), op.column());
    }
}

syntax::ExpressionPtr Parser::parseEquality() {
    syntax::ExpressionPtr left = parseComparison();
    for (;;) {
        const Token op = peek();
        if (op.kind() != Token::Kind::OPERATOR || (op.text() != "==" && op.text() != "!=")) {
            return left;
        }
        next();
        left = boost::make_shared<syntax::Binary>(op.text(), left, parseComparison(), op.line(), op.column());
    }
}

syntax::ExpressionPtr Parser::parseComparison() {
    syntax::ExpressionPtr left = parseAdditive();
    for (;;) {
        const Token op = peek();
        const bool compares = op.kind() == Token::Kind::OPERATOR &&
            (op.text() == "<" || op.text() == "<=" || op.text() == ">" || op.text() == ">=");
        if (!compares) {
            return left;
        }
        next();
        left = boost::make_shared<syntax::Binary>(op.text(), left, parseAdditive(), op.line(), op.column());
    }
}

syntax::ExpressionPtr Parser::parseAdditive() {
    syntax::ExpressionPtr left = parseMultiplicative();
    for (;;) {
        const Token op = peek();
        const bool adds = op.kind() == Token::Kind::OPERATOR && (op.text() == "+" || op.text() == "-");
        if (!adds) {
            return left;
        }
        next();
        left = boost::make_shared<syntax::Binary>(op.text(), left, parseMultiplicative(), op.line(), op.column());
    }
}

syntax::ExpressionPtr Parser::parseMultiplicative() {
    syntax::ExpressionPtr left = parseProduct();
    for (;;) {
        const Token op = peek();
        const bool scales = op.kind() == Token::Kind::OPERATOR && (op.text() == "*" || op.text() == "/");
        if (!scales) {
            return left;
        }
        next();
        left = boost::make_shared<syntax::Binary>(op.text(), left, parseProduct(), op.line(), op.column());
    }
}

syntax::ExpressionPtr Parser::parseProduct() {
    syntax::ExpressionPtr left = parseUnary();
    for (;;) {
        const Token op = peek();
        const bool product = op.kind() == Token::Kind::OPERATOR && (op.text() == "." || op.text() == "^");
        if (!product) {
            return left;
        }
        next();
        left = boost::make_shared<syntax::Binary>(op.text(), left, parseUnary(), op.line(), op.column());
    }
}

syntax::ExpressionPtr Parser::parseUnary() {
    const Token token = peek();
    if (token.kind() == Token::Kind::OPERATOR && (token.text() == "-" || token.text() == "!")) {
        next();
        return boost::make_shared<syntax::Unary>(token.text(), parseUnary(), token.line(), token.column());
    }
    Type type = Type::FLOAT;
    if (parseType(&type)) {
        // a cast, and the place a coordinate space is named: `point "world" (0, 0, 0)`
        // casts and transforms in one
        std::string space;
        if (peek().kind() == Token::Kind::STRING) {
            space = next().text();
        }
        return boost::make_shared<syntax::Cast>(type, space, parseUnary(), token.line(), token.column());
    }
    return parsePostfix();
}

syntax::ExpressionPtr Parser::parsePostfix() {
    syntax::ExpressionPtr expression = parsePrimary();
    for (;;) {
        const Token bracket = peek();
        if (!accept(Token::Kind::PUNCTUATION, "[")) {
            return expression;
        }
        syntax::ExpressionPtr index = parseExpression();
        expect(Token::Kind::PUNCTUATION, "]");
        expression = boost::make_shared<syntax::Index>(expression, index, bracket.line(), bracket.column());
    }
}

syntax::ExpressionPtr Parser::parsePrimary() {
    const Token token = peek();
    if (token.kind() == Token::Kind::NUMBER) {
        next();
        return boost::make_shared<syntax::Number>(token.value(), token.line(), token.column());
    }
    if (token.kind() == Token::Kind::STRING) {
        next();
        return boost::make_shared<syntax::String>(token.text(), token.line(), token.column());
    }
    if (token.kind() == Token::Kind::IDENTIFIER) {
        next();
        if (!at(Token::Kind::PUNCTUATION, "(")) {
            return boost::make_shared<syntax::Variable>(token.text(), token.line(), token.column());
        }
        boost::shared_ptr<syntax::Call> call =
            boost::make_shared<syntax::Call>(token.text(), token.line(), token.column());
        expect(Token::Kind::PUNCTUATION, "(");
        if (!at(Token::Kind::PUNCTUATION, ")")) {
            call->arguments.push_back(parseExpression());
            while (accept(Token::Kind::PUNCTUATION, ",")) {
                call->arguments.push_back(parseExpression());
            }
        }
        expect(Token::Kind::PUNCTUATION, ")");
        return call;
    }
    if (token.kind() == Token::Kind::PUNCTUATION && token.text() == "(") {
        return parseParenthesised();
    }
    throw fail("expected an expression but found " + describe(token), token);
}

syntax::ExpressionPtr Parser::parseParenthesised() {
    const Token opening = expect(Token::Kind::PUNCTUATION, "(");
    syntax::ExpressionPtr first = parseExpression();
    if (!at(Token::Kind::PUNCTUATION, ",")) {
        expect(Token::Kind::PUNCTUATION, ")");
        return first;
    }
    boost::shared_ptr<syntax::Tuple> tuple = boost::make_shared<syntax::Tuple>(opening.line(), opening.column());
    tuple->elements.push_back(first);
    while (accept(Token::Kind::PUNCTUATION, ",")) {
        tuple->elements.push_back(parseExpression());
    }
    expect(Token::Kind::PUNCTUATION, ")");
    return tuple;
}

};  // namespace v3d::render::offline::sl
