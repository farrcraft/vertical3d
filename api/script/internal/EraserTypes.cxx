/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Eraser.h"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

#include "Scanner.h"

namespace v3d::script::internal {

namespace {

/**
 * @return whether a token can name a class member or an object's property, so that a word
 *         before it is a modifier rather than the name
 **/
bool names(const Lexeme& token) {
    return token.kind == Token::Identifier || token.kind == Token::String || token.kind == Token::Number || token.kind == Token::Private ||
        token.is("[") || token.is("*") || token.is("{");
}

/**
 * @return whether a word may come before a class member's name
 **/
bool modifies(const Lexeme& token) {
    return token.kind == Token::Identifier && (Eraser::modifier(token) || token.is("declare") || token.is("abstract") || token.is("static") ||
        token.is("accessor") || token.is("async") || token.is("get") || token.is("set"));
}

};  // namespace

/**
 **/
bool Eraser::modifier(const Lexeme& token) {
    return token.is("public") || token.is("private") || token.is("protected") || token.is("readonly") || token.is("override");
}

/**
 **/
void Eraser::classDeclaration() {
    expect("class");
    if (token_.kind == Token::Identifier && !at("extends") && !at("implements")) {
        advance();
    }
    if (at("<")) {
        const std::size_t begin = token_.begin;
        typeParameters();
        eraseFrom(begin);
    }
    if (at("extends")) {
        advance();
        expression({ .comma = false, .heritage = true });
    }
    if (at("implements")) {
        const std::size_t begin = token_.begin;
        advance();
        typeList();
        eraseFrom(begin);
    }
    expect("{");
    while (!at("}")) {
        if (token_.kind == Token::End) {
            mismatch();
        }
        member();
    }
    expect("}");
}

/**
 **/
void Eraser::member() {
    if (at(";")) {
        advance();
        return;
    }
    const std::size_t start = token_.begin;
    if (at("static") && peek().is("{")) {
        advance();
        block();
        return;
    }
    bool whole = modifiers(start);
    if (at("*")) {
        advance();
    }
    if (indexSignature(start)) {
        return;
    }
    const bool constructor = at("constructor");
    propertyName();
    if ((at("?") || at("!")) && sameLine()) {
        erase(token_.begin, token_.end);
        advance();
    }
    if (at("(") || at("<")) {
        signature(constructor);
        if (at("{")) {
            block();
        } else {
            // a method with no body is an overload or abstract, which is a type
            if (at(";")) {
                advance();
            }
            whole = true;
        }
    } else {
        field();
    }
    if (whole) {
        eraseFrom(start);
    }
}

/**
 **/
bool Eraser::modifiers(std::size_t start) {
    bool whole = false;
    // the run of an erased modifier that opens the member, and whether a modifier kept follows it
    std::optional<std::size_t> opening;
    bool kept = false;
    while (modifies(token_)) {
        const Lexeme next = peek();
        if (!names(next) || next.is("{") || (at("async") && next.newline)) {
            break;
        }
        if (modifier(token_)) {
            if (token_.begin == start) {
                opening = erased_.size();
            }
            erase(token_.begin, token_.end);
        } else if (at("declare") || at("abstract")) {
            whole = true;
        } else if (at("static") || at("accessor")) {
            kept = true;
        }
        advance();
    }
    // a field before the member may end without a semicolon, and then a computed name or a
    // generator's * would continue it, so one stands where the modifier was
    if (opening && !kept && (at("[") || at("*"))) {
        erased_[*opening].first = ';';
    }
    return whole;
}

/**
 **/
bool Eraser::indexSignature(std::size_t start) {
    if (!at("[")) {
        return false;
    }
    // an index signature, as [key: string]: number, is a type
    const Mark from = mark();
    advance();
    const bool signature = token_.kind == Token::Identifier && peek().is(":");
    back(from);
    if (!signature) {
        return false;
    }
    balanced();
    if (at(":")) {
        advance();
        type();
    }
    if (at(";") || at(",")) {
        advance();
    }
    eraseFrom(start);
    return true;
}

/**
 **/
void Eraser::field() {
    if (at(":")) {
        const std::size_t begin = token_.begin;
        advance();
        type();
        eraseFrom(begin);
    }
    if (at("=")) {
        advance();
        expression({ .comma = false });
    }
    terminate();
}

/**
 **/
void Eraser::ambient(std::size_t start) {
    if (at("var") || at("let") || at("const")) {
        advance();
        declarations({});
        if (at(";")) {
            advance();
        }
    } else if (at("function") || (at("async") && peek().is("function"))) {
        if (at("async")) {
            advance();
        }
        function(true, start);
    } else if (at("class")) {
        classDeclaration();
    } else if (at("abstract") && peek().is("class")) {
        advance();
        classDeclaration();
    } else if (at("interface") || at("type")) {
        declarationOfTypes(start);
    } else {
        // a namespace, a module, an enum or the global scope, each a body in braces
        while (!at("{")) {
            if (token_.kind == Token::End || at(";")) {
                mismatch();
            }
            advance();
        }
        balanced();
    }
    eraseFrom(start);
}

/**
 **/
void Eraser::moduleStatement() {
    const std::size_t start = token_.begin;
    if (at("import")) {
        advance();
        const Lexeme next = peek();
        if (at("type") && (next.kind == Token::Identifier || next.is("{") || next.is("*")) && !next.is("from")) {
            typeOnlyStatement(start);
            return;
        }
        if (token_.kind == Token::Identifier && next.is("=")) {
            refuse("import = cannot be erased. Use an import declaration");
        }
        specifiers();
        return;
    }
    expect("export");
    if (at("=")) {
        refuse("export = cannot be erased. Use an export declaration");
    }
    if (at("import")) {
        refuse("export import cannot be erased");
    }
    if (at("as")) {
        typeOnlyStatement(start);
        return;
    }
    if (at("type") && (peek().is("{") || peek().is("*"))) {
        typeOnlyStatement(start);
        return;
    }
    if (at("{") || at("*")) {
        specifiers();
        return;
    }
    if (at("default")) {
        advance();
        if (declarationOfTypes(start)) {
            return;
        }
        if (at("function") || at("class") || at("async")) {
            statement();
        } else {
            expression({ .comma = false });
            terminate();
        }
        return;
    }
    // a declaration of types takes the export with it
    if (declarationOfTypes(start)) {
        return;
    }
    statement();
}

/**
 **/
void Eraser::typeOnlyStatement(std::size_t start) {
    while (!at(";") && token_.kind != Token::End) {
        if (at("{")) {
            balanced();
            continue;
        }
        const bool specifier = token_.kind == Token::String;
        advance();
        if (specifier) {
            break;
        }
    }
    if (at(";")) {
        advance();
    }
    eraseFrom(start);
}

/**
 **/
void Eraser::specifiers() {
    // what an import or export declaration names, in which a name marked type is erased
    while (!at(";") && token_.kind != Token::End) {
        if (at("{")) {
            advance();
            while (!at("}") && token_.kind != Token::End) {
                const std::size_t begin = token_.begin;
                const Lexeme next = peek();
                const bool typeOnly = at("type") && next.kind == Token::Identifier && !next.is("as");
                advance();
                while (!at(",") && !at("}") && token_.kind != Token::End) {
                    advance();
                }
                if (at(",")) {
                    advance();
                }
                if (typeOnly) {
                    eraseFrom(begin);
                }
            }
            expect("}");
            continue;
        }
        const bool specifier = token_.kind == Token::String;
        advance();
        if (specifier && !at("with") && !at("assert")) {
            break;
        }
    }
    terminate();
}

/**
 **/
void Eraser::type(bool conditional) {
    const Level level(this);
    if (at("<")) {
        // a generic function type
        typeParameters();
        balanced();
        expect("=>");
        type();
        return;
    }
    if (at("new") || (at("abstract") && peek().is("new"))) {
        if (at("abstract")) {
            advance();
        }
        advance();
        if (at("<")) {
            typeParameters();
        }
        balanced();
        expect("=>");
        type();
        return;
    }
    if (at("(")) {
        const Mark from = mark();
        balanced();
        if (at("=>")) {
            advance();
            type();
            return;
        }
        back(from);
    }
    unionType();
    if (conditional && at("extends") && sameLine()) {
        advance();
        type(false);
        expect("?");
        type();
        expect(":");
        type();
    }
}

/**
 **/
void Eraser::unionType() {
    if (at("|") || at("&")) {
        advance();
    }
    primaryType();
    while (at("|") || at("&")) {
        advance();
        primaryType();
    }
}

/**
 **/
void Eraser::primaryType() {
    if (typePrefixes()) {
        return;
    }
    if (at("{") || at("[")) {
        balanced();
    } else if (at("(")) {
        advance();
        type();
        expect(")");
    } else if (at("-")) {
        advance();
        if (token_.kind != Token::Number) {
            mismatch();
        }
        advance();
    } else if (token_.kind == Token::Number || token_.kind == Token::String || token_.kind == Token::Template) {
        advance();
    } else if (token_.kind == Token::TemplateHead) {
        templateLiteral(true);
    } else if (token_.kind == Token::Identifier) {
        if (namedType()) {
            return;
        }
    } else {
        mismatch();
    }
    while (at("[") && sameLine()) {
        balanced();
    }
}

/**
 **/
bool Eraser::typePrefixes() {
    for (;;) {
        const Lexeme next = peek();
        const bool operand = next.kind != Token::Punctuator || next.is("[") || next.is("(") || next.is("{") || next.is("<") || next.is("-");
        if ((at("keyof") || at("unique") || at("readonly") || at("typeof")) && operand) {
            advance();
        } else if (at("infer") && next.kind == Token::Identifier) {
            advance();
            advance();
            if (at("extends")) {
                // a constraint, unless the extends begins a conditional type around it
                const Mark from = mark();
                advance();
                type(false);
                if (at("?")) {
                    back(from);
                }
            }
            return true;
        } else if (at("asserts") && next.kind == Token::Identifier && !next.newline) {
            advance();
            advance();
            if (at("is")) {
                advance();
                type();
            }
            return true;
        } else {
            return false;
        }
    }
}

/**
 **/
bool Eraser::namedType() {
    const bool imported = at("import");
    advance();
    if (imported && at("(")) {
        balanced();
    }
    while (at(".")) {
        advance();
        if (token_.kind != Token::Identifier) {
            mismatch();
        }
        advance();
    }
    if (at("<")) {
        balanced();
    }
    if (at("is") && sameLine()) {
        // a type predicate, as value is string
        advance();
        type();
        return true;
    }
    return false;
}

/**
 **/
void Eraser::typeParameters() {
    if (!at("<")) {
        mismatch();
    }
    balanced();
}

/**
 **/
void Eraser::balanced() {
    // the closer each open bracket waits for; a template's substitution waits for a }
    std::vector<std::string_view> waiting;
    const bool angled = at("<");
    do {
        if (token_.kind == Token::End) {
            mismatch();
        }
        if (const std::optional<std::string_view> closer = opens(angled)) {
            waiting.push_back(*closer);
        } else if (!waiting.empty() && waiting.back() == "${" && at("}")) {
            token_ = scanner_.resume(token_);
            if (token_.kind == Token::Unclosed) {
                refuse("a template here is not closed");
            }
            if (token_.kind == Token::Template) {
                waiting.pop_back();
            }
        } else if (at(")") || at("]") || at("}") || (at(">") && angled)) {
            if (waiting.empty() || waiting.back() != token_.text) {
                mismatch();
            }
            waiting.pop_back();
        } else if (at(";") && !waiting.empty() && waiting.back() == ">") {
            mismatch();
        }
        advance();
    } while (!waiting.empty());
}

/**
 **/
std::optional<std::string_view> Eraser::opens(bool angled) const {
    if (at("(")) {
        return ")";
    }
    if (at("[")) {
        return "]";
    }
    if (at("{")) {
        return "}";
    }
    if (at("<") && angled) {
        return ">";
    }
    if (token_.kind == Token::TemplateHead) {
        return "${";
    }
    return std::nullopt;
}

/**
 **/
void Eraser::typeList() {
    type();
    while (at(",")) {
        advance();
        type();
    }
}

};  // namespace v3d::script::internal
