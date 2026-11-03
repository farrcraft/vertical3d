/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Eraser.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include "Scanner.h"

namespace v3d::editor::script::internal {

namespace {

/**
 * Why an enum is refused.
 **/
constexpr std::string_view ENUMS = "an enum cannot be erased, so a script cannot hold one. An object of constants can stand in for it";

};  // namespace

/**
 **/
Eraser::Eraser(std::string_view source) :
    source_(source),
    scanner_(source) {
}

/**
 **/
std::string Eraser::run() {
    try {
        advance();
        statements();
        if (token_.kind != Token::End) {
            mismatch();
        }
    } catch (const Mismatch& stopped) {
        where_ = stopped.at;
        why_ = "the script cannot be read here";
        return {};
    } catch (const Refusal& refused) {
        where_ = refused.at;
        why_ = refused.message;
        return {};
    }
    std::string code(source_);
    for (const Run& run : erased_) {
        for (std::size_t at = run.begin; at < run.end; ++at) {
            if (code[at] != '\n' && code[at] != '\r') {
                code[at] = ' ';
            }
        }
        if (run.end > run.begin) {
            code[run.begin] = run.first == ' ' ? code[run.begin] : run.first;
            code[run.end - 1] = run.last == ' ' ? code[run.end - 1] : run.last;
        }
    }
    return code;
}

/**
 **/
Eraser::Level::Level(Eraser* eraser) : eraser_(eraser) {
    if (++eraser_->depth_ > NESTING) {
        --eraser_->depth_;
        eraser_->refuse("the script nests too deeply here");
    }
}

/**
 **/
Eraser::Level::~Level() {
    --eraser_->depth_;
}

/**
 **/
std::size_t Eraser::where() const noexcept {
    return where_;
}

/**
 **/
const std::string& Eraser::why() const noexcept {
    return why_;
}

/**
 **/
void Eraser::advance() {
    previous_ = token_;
    token_ = scanner_.next();
    if (token_.kind == Token::Unclosed) {
        refuse("a comment, string or template here is not closed");
    }
}

/**
 **/
bool Eraser::at(std::string_view written) const noexcept {
    return token_.is(written);
}

/**
 **/
bool Eraser::sameLine() const noexcept {
    return !token_.newline;
}

/**
 **/
void Eraser::expect(std::string_view written) {
    if (!at(written)) {
        mismatch();
    }
    advance();
}

/**
 **/
Lexeme Eraser::peek() {
    const Mark from = mark();
    advance();
    const Lexeme next = token_;
    back(from);
    return next;
}

/**
 **/
Eraser::Mark Eraser::mark() const {
    return { .position = scanner_.position(), .token = token_, .previous = previous_, .erased = erased_.size() };
}

/**
 **/
void Eraser::back(const Mark& to) {
    scanner_.position(to.position);
    token_ = to.token;
    previous_ = to.previous;
    erased_.resize(to.erased);
}

/**
 **/
void Eraser::mismatch() const {
    throw Mismatch{ token_.begin };
}

/**
 **/
void Eraser::refuse(const std::string& message) const {
    refuse(message, token_.begin);
}

/**
 **/
void Eraser::refuse(const std::string& message, std::size_t at) {
    throw Refusal{ at, message };
}

/**
 **/
void Eraser::erase(std::size_t begin, std::size_t end, char last) {
    erased_.push_back({ .begin = begin, .end = end, .last = last });
}

/**
 **/
void Eraser::eraseFrom(std::size_t begin) {
    erase(begin, previous_.end);
}

/**
 **/
void Eraser::statements() {
    while (token_.kind != Token::End && !at("}")) {
        statement();
    }
}

/**
 **/
void Eraser::statement() {
    const Level level(this);
    if (at("{")) {
        block();
        return;
    }
    if (at(";")) {
        advance();
        return;
    }
    if (token_.kind == Token::Identifier) {
        if (declarationOfTypes(token_.begin) || declaration() || control() || jump()) {
            return;
        }
        if (peek().is(":")) {
            // a label
            advance();
            advance();
            statement();
            return;
        }
    }
    expression({});
    terminate();
}

/**
 **/
bool Eraser::declaration() {
    const Lexeme first = token_;
    const Lexeme next = peek();
    const bool joined = !next.newline;
    if (at("abstract") && next.is("class") && joined) {
        erase(first.begin, first.end);
        advance();
        classDeclaration();
    } else if (at("var") || at("let") || at("const")) {
        if (at("const") && next.is("enum") && joined) {
            refuse(std::string(ENUMS));
        }
        advance();
        declarations({});
        terminate();
    } else if (at("function") || (at("async") && next.is("function") && joined)) {
        if (at("async")) {
            advance();
        }
        function(true, first.begin);
    } else if (at("class")) {
        classDeclaration();
    } else if ((at("import") && !next.is("(") && !next.is(".")) || at("export")) {
        moduleStatement();
    } else {
        return false;
    }
    return true;
}

/**
 **/
bool Eraser::control() {
    if (at("if")) {
        advance();
        parenthesised();
        statement();
        if (at("else")) {
            advance();
            statement();
        }
    } else if (at("while") || at("with")) {
        advance();
        parenthesised();
        statement();
    } else if (at("do")) {
        advance();
        statement();
        expect("while");
        parenthesised();
        if (at(";")) {
            advance();
        }
    } else if (at("for")) {
        forStatement();
    } else if (at("switch")) {
        switchStatement();
    } else if (at("try")) {
        tryStatement();
    } else {
        return false;
    }
    return true;
}

/**
 **/
bool Eraser::jump() {
    if (at("return") || at("throw")) {
        advance();
        if (!at(";") && !at("}") && token_.kind != Token::End && sameLine()) {
            expression({});
        }
    } else if (at("break") || at("continue")) {
        advance();
        if (token_.kind == Token::Identifier && sameLine()) {
            advance();
        }
    } else if (at("debugger")) {
        advance();
    } else {
        return false;
    }
    terminate();
    return true;
}

/**
 **/
bool Eraser::declarationOfTypes(std::size_t start) {
    if (token_.kind != Token::Identifier) {
        return false;
    }
    const Lexeme next = peek();
    const bool joined = !next.newline;
    const bool named = next.kind == Token::Identifier && joined;
    if (at("interface") && named) {
        interfaceDeclaration(start);
        return true;
    }
    if (at("type") && named) {
        return typeAlias(start);
    }
    if (at("declare") && named) {
        advance();
        ambient(start);
        return true;
    }
    if (at("enum") && named) {
        refuse(std::string(ENUMS));
    }
    if ((at("namespace") || at("module")) && joined && (next.kind == Token::Identifier || next.kind == Token::String)) {
        refuse("a namespace cannot be erased, so a script cannot hold one");
    }
    return false;
}

/**
 **/
void Eraser::interfaceDeclaration(std::size_t start) {
    expect("interface");
    advance();
    if (at("<")) {
        typeParameters();
    }
    if (at("extends")) {
        advance();
        typeList();
    }
    if (!at("{")) {
        mismatch();
    }
    balanced();
    eraseFrom(start);
}

/**
 **/
bool Eraser::typeAlias(std::size_t start) {
    const Mark from = mark();
    advance();
    advance();
    if (!at("=") && !at("<")) {
        // type is a name here, as in type = 2
        back(from);
        return false;
    }
    if (at("<")) {
        typeParameters();
    }
    expect("=");
    type();
    if (at(";")) {
        advance();
    }
    eraseFrom(start);
    return true;
}

/**
 **/
void Eraser::block() {
    expect("{");
    statements();
    expect("}");
}

/**
 **/
void Eraser::terminate() {
    if (at(";")) {
        advance();
    } else if (!at("}") && token_.kind != Token::End && sameLine()) {
        mismatch();
    }
}

/**
 **/
void Eraser::declarations(const Flags& flags) {
    for (;;) {
        binding();
        if (at("!") && sameLine()) {
            erase(token_.begin, token_.end);
            advance();
        }
        if (at(":")) {
            const std::size_t start = token_.begin;
            advance();
            type();
            eraseFrom(start);
        }
        if (at("=")) {
            advance();
            expression({ .comma = false, .in = flags.in });
        }
        if (!at(",")) {
            return;
        }
        advance();
    }
}

/**
 **/
void Eraser::binding() {
    if (at("{")) {
        objectLiteral();
    } else if (at("[")) {
        arrayLiteral();
    } else if (token_.kind == Token::Identifier) {
        advance();
    } else {
        mismatch();
    }
}

/**
 **/
void Eraser::function(bool declaration, std::size_t start) {
    expect("function");
    if (at("*")) {
        advance();
    }
    if (token_.kind == Token::Identifier) {
        advance();
    }
    signature(false);
    if (at("{")) {
        block();
        return;
    }
    if (!declaration) {
        mismatch();
    }
    // a signature with no body is an overload, which is a type
    if (at(";")) {
        advance();
    }
    eraseFrom(start);
}

/**
 **/
void Eraser::signature(bool constructor) {
    if (at("<")) {
        const std::size_t start = token_.begin;
        typeParameters();
        eraseFrom(start);
    }
    parameters(constructor);
    if (at(":")) {
        const std::size_t start = token_.begin;
        advance();
        type();
        eraseFrom(start);
    }
}

/**
 **/
void Eraser::parameters(bool constructor) {
    expect("(");
    while (!at(")")) {
        if (parameter(constructor)) {
            continue;
        }
        if (!at(",")) {
            break;
        }
        advance();
    }
    expect(")");
}

/**
 **/
bool Eraser::parameter(bool constructor) {
    if (constructor && modifier(token_)) {
        const Lexeme next = peek();
        if (next.kind == Token::Identifier || next.is("{") || next.is("[")) {
            refuse("a parameter property cannot be erased. Declare the field, and assign it in the constructor");
        }
    }
    if (at("this") && peek().is(":")) {
        // a function's this parameter is a type, and goes with the comma after it
        const std::size_t start = token_.begin;
        advance();
        advance();
        type();
        if (at(",")) {
            advance();
        }
        eraseFrom(start);
        return true;
    }
    if (at("...")) {
        advance();
    }
    binding();
    if (at("?")) {
        erase(token_.begin, token_.end);
        advance();
    }
    if (at(":")) {
        const std::size_t start = token_.begin;
        advance();
        type();
        eraseFrom(start);
    }
    if (at("=")) {
        advance();
        expression({ .comma = false });
    }
    return false;
}

/**
 **/
void Eraser::forStatement() {
    expect("for");
    if (at("await")) {
        advance();
    }
    expect("(");
    if (at("var") || at("let") || at("const")) {
        advance();
        declarations({ .in = false });
    } else if (!at(";")) {
        expression({ .in = false });
    }
    if (at("of") || at("in")) {
        advance();
        expression({});
    } else {
        expect(";");
        if (!at(";")) {
            expression({});
        }
        expect(";");
        if (!at(")")) {
            expression({});
        }
    }
    expect(")");
    statement();
}

/**
 **/
void Eraser::switchStatement() {
    expect("switch");
    parenthesised();
    expect("{");
    while (!at("}") && token_.kind != Token::End) {
        if (at("case")) {
            advance();
            expression({});
            expect(":");
        } else if (at("default") && peek().is(":")) {
            advance();
            advance();
        } else {
            statement();
        }
    }
    expect("}");
}

/**
 **/
void Eraser::tryStatement() {
    expect("try");
    block();
    if (at("catch")) {
        advance();
        if (at("(")) {
            advance();
            binding();
            if (at(":")) {
                const std::size_t start = token_.begin;
                advance();
                type();
                eraseFrom(start);
            }
            expect(")");
        }
        block();
    }
    if (at("finally")) {
        advance();
        block();
    }
}

};  // namespace v3d::editor::script::internal
