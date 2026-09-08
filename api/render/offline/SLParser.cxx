/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SLParser.h"

#include <istream>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::offline {

namespace {

std::string position(const SLToken & token) {
    return " at line " + std::to_string(token.line()) + ", column " + std::to_string(token.column());
}

/**
 * What a token is, for a diagnostic. An END has no text to quote, and "end of source" is
 * what a reader needs to hear rather than an empty pair of quotes.
 **/
std::string describe(const SLToken & token) {
    switch (token.kind()) {
        case SLToken::Kind::END:
            return "end of source";
        case SLToken::Kind::NUMBER:
            return "the number " + std::to_string(token.value());
        case SLToken::Kind::STRING:
            return "the string \"" + token.text() + "\"";
        default:
            return "'" + token.text() + "'";
    }
}

bool shaderType(const SLToken & token, SLShaderType* type) {
    if (token.kind() != SLToken::Kind::KEYWORD) {
        return false;
    }
    if (token.text() == "surface") {
        *type = SLShaderType::SURFACE;
    } else if (token.text() == "light") {
        *type = SLShaderType::LIGHT;
    } else if (token.text() == "displacement") {
        *type = SLShaderType::DISPLACEMENT;
    } else if (token.text() == "volume") {
        *type = SLShaderType::VOLUME;
    } else if (token.text() == "imager") {
        *type = SLShaderType::IMAGER;
    } else {
        return false;
    }
    return true;
}

bool assignment(const SLToken & token) {
    if (token.kind() != SLToken::Kind::OPERATOR) {
        return false;
    }
    const std::string & text = token.text();
    return text == "=" || text == "+=" || text == "-=" || text == "*=" || text == "/=";
}

};  // namespace

SLParser::SLParser(std::istream & stream) : lexer_(stream) {
}

const std::string & SLParser::error() const {
    return error_;
}

SLParser::Failure SLParser::fail(const std::string & message, const SLToken & token) {
    if (error_.empty()) {
        error_ = message + position(token);
    }
    return Failure();
}

const SLToken & SLParser::peek() {
    const SLToken & token = lexer_.peek();
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

SLToken SLParser::next() {
    peek();
    return lexer_.next();
}

bool SLParser::at(SLToken::Kind kind, const std::string & text) {
    const SLToken & token = peek();
    return token.kind() == kind && token.text() == text;
}

bool SLParser::accept(SLToken::Kind kind, const std::string & text) {
    if (!at(kind, text)) {
        return false;
    }
    next();
    return true;
}

SLToken SLParser::expect(SLToken::Kind kind, const std::string & text) {
    const SLToken & token = peek();
    if (token.kind() != kind || token.text() != text) {
        throw fail("expected '" + text + "' but found " + describe(token), token);
    }
    return next();
}

SLToken SLParser::expectKind(SLToken::Kind kind, const char* what) {
    const SLToken & token = peek();
    if (token.kind() != kind) {
        throw fail(std::string("expected ") + what + " but found " + describe(token), token);
    }
    return next();
}

std::vector<SLShaderPtr> SLParser::parse() {
    std::vector<SLShaderPtr> shaders;
    try {
        while (peek().kind() != SLToken::Kind::END) {
            shaders.push_back(parseShader());
        }
    } catch (const Failure &) {
        // a parse that fails yields no program at all: half a shader is worse than none,
        // because a renderer would run it
        return std::vector<SLShaderPtr>();
    }
    return shaders;
}

SLShaderPtr SLParser::parseShader() {
    const SLToken opening = peek();
    SLShaderType type = SLShaderType::SURFACE;
    if (!shaderType(opening, &type)) {
        throw fail("expected a shader type but found " + describe(opening), opening);
    }
    SLShaderPtr shader = boost::make_shared<SLShader>();
    shader->type = type;
    shader->line = opening.line();
    shader->column = opening.column();
    next();

    shader->name = expectKind(SLToken::Kind::IDENTIFIER, "a shader name").text();
    expect(SLToken::Kind::PUNCTUATION, "(");
    if (!at(SLToken::Kind::PUNCTUATION, ")")) {
        parseParameters(&shader->parameters, true);
    }
    expect(SLToken::Kind::PUNCTUATION, ")");

    const SLToken brace = expect(SLToken::Kind::PUNCTUATION, "{");
    shader->body = boost::make_shared<SLBlock>(brace.line(), brace.column());
    while (!at(SLToken::Kind::PUNCTUATION, "}")) {
        if (peek().kind() == SLToken::Kind::END) {
            throw fail("expected '}' but found end of source", peek());
        }
        // a storage class can only begin a declaration; a bare type keyword begins either a
        // declaration or a function, and the token after the name is what tells them apart
        const SLStorage storage = parseStorage();
        SLType declared = SLType::FLOAT;
        if (storage != SLStorage::UNSPECIFIED) {
            const SLToken token = peek();
            if (!parseType(&declared)) {
                throw fail("expected a type but found " + describe(token), token);
            }
            const SLToken name = expectKind(SLToken::Kind::IDENTIFIER, "a name");
            shader->body->statements.push_back(parseDeclaration(storage, declared, name));
            expect(SLToken::Kind::PUNCTUATION, ";");
            continue;
        }
        const SLToken typeToken = peek();
        if (!parseType(&declared)) {
            shader->body->statements.push_back(parseStatement());
            continue;
        }
        const SLToken name = expectKind(SLToken::Kind::IDENTIFIER, "a name");
        if (!at(SLToken::Kind::PUNCTUATION, "(")) {
            shader->body->statements.push_back(parseDeclaration(storage, declared, name));
            expect(SLToken::Kind::PUNCTUATION, ";");
            continue;
        }
        SLFunction function;
        function.type = declared;
        function.name = name.text();
        function.line = typeToken.line();
        function.column = typeToken.column();
        expect(SLToken::Kind::PUNCTUATION, "(");
        if (!at(SLToken::Kind::PUNCTUATION, ")")) {
            parseParameters(&function.parameters, false);
        }
        expect(SLToken::Kind::PUNCTUATION, ")");
        function.body = parseBlock();
        shader->functions.push_back(function);
    }
    expect(SLToken::Kind::PUNCTUATION, "}");
    return shader;
}

void SLParser::parseParameters(std::vector<SLParameter>* parameters, bool defaults) {
    for (;;) {
        const SLToken opening = peek();
        SLParameter parameter;
        parameter.line = opening.line();
        parameter.column = opening.column();

        // "output varying color Ci = 0" is how SL writes it, but neither order is worth
        // rejecting, and 'output' is an identifier rather than a keyword
        for (;;) {
            if (accept(SLToken::Kind::IDENTIFIER, "output")) {
                parameter.output = true;
                continue;
            }
            const SLStorage storage = parseStorage();
            if (storage != SLStorage::UNSPECIFIED) {
                parameter.storage = storage;
                continue;
            }
            break;
        }

        const SLToken typeToken = peek();
        if (!parseType(&parameter.type)) {
            throw fail("expected a parameter type but found " + describe(typeToken), typeToken);
        }
        parameter.name = expectKind(SLToken::Kind::IDENTIFIER, "a parameter name").text();
        if (defaults) {
            // SL has no uninitialised shader parameter: the default is what a scene that
            // does not mention the parameter gets, so a missing one is an error rather
            // than a zero
            expect(SLToken::Kind::OPERATOR, "=");
            parameter.defaultValue = parseExpression();
        }
        parameters->push_back(parameter);

        // a shader's parameters are separated by semicolons and a function's formals by
        // commas, and a file written the other way round is not worth refusing
        if (!accept(SLToken::Kind::PUNCTUATION, ";") && !accept(SLToken::Kind::PUNCTUATION, ",")) {
            return;
        }
        if (at(SLToken::Kind::PUNCTUATION, ")")) {
            return;
        }
    }
}

bool SLParser::parseType(SLType* type) {
    const SLToken & token = peek();
    if (token.kind() != SLToken::Kind::KEYWORD) {
        return false;
    }
    const std::string & text = token.text();
    if (text == "float") {
        *type = SLType::FLOAT;
    } else if (text == "point") {
        *type = SLType::POINT;
    } else if (text == "vector") {
        *type = SLType::VECTOR;
    } else if (text == "normal") {
        *type = SLType::NORMAL;
    } else if (text == "color") {
        *type = SLType::COLOR;
    } else if (text == "matrix") {
        *type = SLType::MATRIX;
    } else if (text == "string") {
        *type = SLType::STRING;
    } else if (text == "void") {
        *type = SLType::VOID;
    } else {
        return false;
    }
    next();
    return true;
}

SLStorage SLParser::parseStorage() {
    if (accept(SLToken::Kind::KEYWORD, "uniform")) {
        return SLStorage::UNIFORM;
    }
    if (accept(SLToken::Kind::KEYWORD, "varying")) {
        return SLStorage::VARYING;
    }
    return SLStorage::UNSPECIFIED;
}

SLBlockPtr SLParser::parseBlock() {
    const SLToken brace = expect(SLToken::Kind::PUNCTUATION, "{");
    SLBlockPtr block = boost::make_shared<SLBlock>(brace.line(), brace.column());
    while (!at(SLToken::Kind::PUNCTUATION, "}")) {
        if (peek().kind() == SLToken::Kind::END) {
            throw fail("expected '}' but found end of source", peek());
        }
        block->statements.push_back(parseStatement());
    }
    expect(SLToken::Kind::PUNCTUATION, "}");
    return block;
}

SLStatementPtr SLParser::parseStatement() {
    const SLToken token = peek();
    if (token.kind() == SLToken::Kind::PUNCTUATION && token.text() == "{") {
        return parseBlock();
    }
    if (accept(SLToken::Kind::PUNCTUATION, ";")) {
        // an empty statement is an empty block, which is what it does
        return boost::make_shared<SLBlock>(token.line(), token.column());
    }
    if (token.kind() == SLToken::Kind::KEYWORD) {
        return parseKeywordStatement(token);
    }
    SLStatementPtr statement = parseSimpleStatement();
    expect(SLToken::Kind::PUNCTUATION, ";");
    return statement;
}

SLStatementPtr SLParser::parseKeywordStatement(const SLToken & keyword) {
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
        return parseLighting(SLLighting::Construct::ILLUMINANCE);
    }
    if (text == "illuminate") {
        return parseLighting(SLLighting::Construct::ILLUMINATE);
    }
    if (text == "solar") {
        return parseLighting(SLLighting::Construct::SOLAR);
    }
    if (text == "else") {
        throw fail("'else' without a matching 'if'", keyword);
    }
    return parseLocalDeclaration();
}

SLStatementPtr SLParser::parseConditional() {
    const SLToken keyword = expect(SLToken::Kind::KEYWORD, "if");
    expect(SLToken::Kind::PUNCTUATION, "(");
    SLExpressionPtr condition = parseExpression();
    expect(SLToken::Kind::PUNCTUATION, ")");
    SLStatementPtr whenTrue = parseStatement();
    SLStatementPtr whenFalse;
    // an else binds to the nearest if, which is what taking it here rather than unwinding
    // to an outer one does
    if (accept(SLToken::Kind::KEYWORD, "else")) {
        whenFalse = parseStatement();
    }
    return boost::make_shared<SLConditional>(condition, whenTrue, whenFalse,
        keyword.line(), keyword.column());
}

SLStatementPtr SLParser::parseWhile() {
    const SLToken keyword = expect(SLToken::Kind::KEYWORD, "while");
    expect(SLToken::Kind::PUNCTUATION, "(");
    SLExpressionPtr condition = parseExpression();
    expect(SLToken::Kind::PUNCTUATION, ")");
    return boost::make_shared<SLWhile>(condition, parseStatement(), keyword.line(), keyword.column());
}

SLStatementPtr SLParser::parseFor() {
    const SLToken keyword = expect(SLToken::Kind::KEYWORD, "for");
    boost::shared_ptr<SLFor> loop = boost::make_shared<SLFor>(keyword.line(), keyword.column());
    expect(SLToken::Kind::PUNCTUATION, "(");
    // any of the three heads may be empty, which is what a null one on the node is
    if (!at(SLToken::Kind::PUNCTUATION, ";")) {
        loop->initialiser = parseSimpleStatement();
    }
    expect(SLToken::Kind::PUNCTUATION, ";");
    if (!at(SLToken::Kind::PUNCTUATION, ";")) {
        loop->condition = parseExpression();
    }
    expect(SLToken::Kind::PUNCTUATION, ";");
    if (!at(SLToken::Kind::PUNCTUATION, ")")) {
        loop->step = parseSimpleStatement();
    }
    expect(SLToken::Kind::PUNCTUATION, ")");
    loop->body = parseStatement();
    return loop;
}

SLStatementPtr SLParser::parseJump() {
    const SLToken keyword = next();
    SLJump::Where where = SLJump::Where::RETURN;
    SLExpressionPtr value;
    if (keyword.text() == "break") {
        where = SLJump::Where::BREAK;
    } else if (keyword.text() == "continue") {
        where = SLJump::Where::CONTINUE;
    } else if (!at(SLToken::Kind::PUNCTUATION, ";")) {
        value = parseExpression();
    }
    expect(SLToken::Kind::PUNCTUATION, ";");
    return boost::make_shared<SLJump>(where, value, keyword.line(), keyword.column());
}

SLStatementPtr SLParser::parseLocalDeclaration() {
    const SLStorage storage = parseStorage();
    SLType type = SLType::FLOAT;
    const SLToken typeToken = peek();
    if (!parseType(&type)) {
        throw fail("expected a type but found " + describe(typeToken), typeToken);
    }
    const SLToken name = expectKind(SLToken::Kind::IDENTIFIER, "a name");
    if (at(SLToken::Kind::PUNCTUATION, "(")) {
        throw fail("a function may only be defined at the top of a shader body", name);
    }
    SLStatementPtr declaration = parseDeclaration(storage, type, name);
    expect(SLToken::Kind::PUNCTUATION, ";");
    return declaration;
}

SLStatementPtr SLParser::parseDeclaration(SLStorage storage, SLType type, const SLToken & first) {
    boost::shared_ptr<SLDeclaration> declaration =
        boost::make_shared<SLDeclaration>(storage, type, first.line(), first.column());
    SLToken name = first;
    for (;;) {
        SLDeclarator declarator;
        declarator.name = name.text();
        declarator.line = name.line();
        declarator.column = name.column();
        if (accept(SLToken::Kind::OPERATOR, "=")) {
            declarator.initialiser = parseExpression();
        }
        declaration->declarators.push_back(declarator);
        if (!accept(SLToken::Kind::PUNCTUATION, ",")) {
            return declaration;
        }
        name = expectKind(SLToken::Kind::IDENTIFIER, "a name");
    }
}

SLStatementPtr SLParser::parseSimpleStatement() {
    const SLToken opening = peek();
    SLExpressionPtr expression = parseExpression();
    if (!assignment(peek())) {
        return boost::make_shared<SLExpressionStatement>(expression, opening.line(), opening.column());
    }
    const SLToken op = next();
    SLExpressionPtr value = parseExpression();
    return boost::make_shared<SLAssignment>(op.text(), expression, value, opening.line(), opening.column());
}

SLStatementPtr SLParser::parseLighting(SLLighting::Construct construct) {
    const SLToken keyword = next();
    boost::shared_ptr<SLLighting> lighting =
        boost::make_shared<SLLighting>(construct, keyword.line(), keyword.column());
    expect(SLToken::Kind::PUNCTUATION, "(");
    if (!at(SLToken::Kind::PUNCTUATION, ")")) {
        lighting->arguments.push_back(parseExpression());
        while (accept(SLToken::Kind::PUNCTUATION, ",")) {
            lighting->arguments.push_back(parseExpression());
        }
    }
    expect(SLToken::Kind::PUNCTUATION, ")");
    lighting->body = parseStatement();
    return lighting;
}

SLExpressionPtr SLParser::parseExpression() {
    return parseTernary();
}

SLExpressionPtr SLParser::parseTernary() {
    SLExpressionPtr condition = parseLogicalOr();
    const SLToken question = peek();
    if (!accept(SLToken::Kind::OPERATOR, "?")) {
        return condition;
    }
    // right associative, so the arms are ternaries themselves rather than one level down
    SLExpressionPtr whenTrue = parseTernary();
    expect(SLToken::Kind::OPERATOR, ":");
    SLExpressionPtr whenFalse = parseTernary();
    return boost::make_shared<SLTernary>(condition, whenTrue, whenFalse, question.line(), question.column());
}

SLExpressionPtr SLParser::parseLogicalOr() {
    SLExpressionPtr left = parseLogicalAnd();
    for (;;) {
        const SLToken op = peek();
        if (!accept(SLToken::Kind::OPERATOR, "||")) {
            return left;
        }
        left = boost::make_shared<SLBinary>(op.text(), left, parseLogicalAnd(), op.line(), op.column());
    }
}

SLExpressionPtr SLParser::parseLogicalAnd() {
    SLExpressionPtr left = parseEquality();
    for (;;) {
        const SLToken op = peek();
        if (!accept(SLToken::Kind::OPERATOR, "&&")) {
            return left;
        }
        left = boost::make_shared<SLBinary>(op.text(), left, parseEquality(), op.line(), op.column());
    }
}

SLExpressionPtr SLParser::parseEquality() {
    SLExpressionPtr left = parseComparison();
    for (;;) {
        const SLToken op = peek();
        if (op.kind() != SLToken::Kind::OPERATOR || (op.text() != "==" && op.text() != "!=")) {
            return left;
        }
        next();
        left = boost::make_shared<SLBinary>(op.text(), left, parseComparison(), op.line(), op.column());
    }
}

SLExpressionPtr SLParser::parseComparison() {
    SLExpressionPtr left = parseAdditive();
    for (;;) {
        const SLToken op = peek();
        const bool compares = op.kind() == SLToken::Kind::OPERATOR &&
            (op.text() == "<" || op.text() == "<=" || op.text() == ">" || op.text() == ">=");
        if (!compares) {
            return left;
        }
        next();
        left = boost::make_shared<SLBinary>(op.text(), left, parseAdditive(), op.line(), op.column());
    }
}

SLExpressionPtr SLParser::parseAdditive() {
    SLExpressionPtr left = parseMultiplicative();
    for (;;) {
        const SLToken op = peek();
        const bool adds = op.kind() == SLToken::Kind::OPERATOR && (op.text() == "+" || op.text() == "-");
        if (!adds) {
            return left;
        }
        next();
        left = boost::make_shared<SLBinary>(op.text(), left, parseMultiplicative(), op.line(), op.column());
    }
}

SLExpressionPtr SLParser::parseMultiplicative() {
    SLExpressionPtr left = parseProduct();
    for (;;) {
        const SLToken op = peek();
        const bool scales = op.kind() == SLToken::Kind::OPERATOR && (op.text() == "*" || op.text() == "/");
        if (!scales) {
            return left;
        }
        next();
        left = boost::make_shared<SLBinary>(op.text(), left, parseProduct(), op.line(), op.column());
    }
}

SLExpressionPtr SLParser::parseProduct() {
    SLExpressionPtr left = parseUnary();
    for (;;) {
        const SLToken op = peek();
        const bool product = op.kind() == SLToken::Kind::OPERATOR && (op.text() == "." || op.text() == "^");
        if (!product) {
            return left;
        }
        next();
        left = boost::make_shared<SLBinary>(op.text(), left, parseUnary(), op.line(), op.column());
    }
}

SLExpressionPtr SLParser::parseUnary() {
    const SLToken token = peek();
    if (token.kind() == SLToken::Kind::OPERATOR && (token.text() == "-" || token.text() == "!")) {
        next();
        return boost::make_shared<SLUnary>(token.text(), parseUnary(), token.line(), token.column());
    }
    SLType type = SLType::FLOAT;
    if (parseType(&type)) {
        // a cast, and the place a coordinate space is named: `point "world" (0, 0, 0)`
        // casts and transforms in one
        std::string space;
        if (peek().kind() == SLToken::Kind::STRING) {
            space = next().text();
        }
        return boost::make_shared<SLCast>(type, space, parseUnary(), token.line(), token.column());
    }
    return parsePostfix();
}

SLExpressionPtr SLParser::parsePostfix() {
    SLExpressionPtr expression = parsePrimary();
    for (;;) {
        const SLToken bracket = peek();
        if (!accept(SLToken::Kind::PUNCTUATION, "[")) {
            return expression;
        }
        SLExpressionPtr index = parseExpression();
        expect(SLToken::Kind::PUNCTUATION, "]");
        expression = boost::make_shared<SLIndex>(expression, index, bracket.line(), bracket.column());
    }
}

SLExpressionPtr SLParser::parsePrimary() {
    const SLToken token = peek();
    if (token.kind() == SLToken::Kind::NUMBER) {
        next();
        return boost::make_shared<SLNumber>(token.value(), token.line(), token.column());
    }
    if (token.kind() == SLToken::Kind::STRING) {
        next();
        return boost::make_shared<SLString>(token.text(), token.line(), token.column());
    }
    if (token.kind() == SLToken::Kind::IDENTIFIER) {
        next();
        if (!at(SLToken::Kind::PUNCTUATION, "(")) {
            return boost::make_shared<SLVariable>(token.text(), token.line(), token.column());
        }
        boost::shared_ptr<SLCall> call =
            boost::make_shared<SLCall>(token.text(), token.line(), token.column());
        expect(SLToken::Kind::PUNCTUATION, "(");
        if (!at(SLToken::Kind::PUNCTUATION, ")")) {
            call->arguments.push_back(parseExpression());
            while (accept(SLToken::Kind::PUNCTUATION, ",")) {
                call->arguments.push_back(parseExpression());
            }
        }
        expect(SLToken::Kind::PUNCTUATION, ")");
        return call;
    }
    if (token.kind() == SLToken::Kind::PUNCTUATION && token.text() == "(") {
        return parseParenthesised();
    }
    throw fail("expected an expression but found " + describe(token), token);
}

SLExpressionPtr SLParser::parseParenthesised() {
    const SLToken opening = expect(SLToken::Kind::PUNCTUATION, "(");
    SLExpressionPtr first = parseExpression();
    if (!at(SLToken::Kind::PUNCTUATION, ",")) {
        expect(SLToken::Kind::PUNCTUATION, ")");
        return first;
    }
    boost::shared_ptr<SLTuple> tuple = boost::make_shared<SLTuple>(opening.line(), opening.column());
    tuple->elements.push_back(first);
    while (accept(SLToken::Kind::PUNCTUATION, ",")) {
        tuple->elements.push_back(parseExpression());
    }
    expect(SLToken::Kind::PUNCTUATION, ")");
    return tuple;
}

};  // namespace v3d::render::offline
