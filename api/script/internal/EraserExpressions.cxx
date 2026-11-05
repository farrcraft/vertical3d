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

namespace v3d::script::internal {

/**
 **/
void Eraser::expression(const Flags& flags) {
    const Level level(this);
    while (operand(flags) && operation(flags)) {
    }
}

/**
 **/
bool Eraser::operand(const Flags& flags) {
    if (prefixes()) {
        return true;
    }
    switch (token_.kind) {
        case Token::Number:
        case Token::String:
        case Token::Template:
        case Token::Regex:
        case Token::Private:
            advance();
            break;
        case Token::TemplateHead:
            templateLiteral(false);
            break;
        case Token::Identifier:
            named(flags);
            break;
        case Token::Punctuator:
            punctuated(flags);
            break;
        case Token::End:
        case Token::TemplateMiddle:
        case Token::Unclosed:
            mismatch();
    }
    return true;
}

/**
 **/
bool Eraser::prefixes() {
    for (;;) {
        if (at("!") || at("~") || at("+") || at("-") || at("++") || at("--") || at("...") || at("typeof") || at("void") || at("delete") ||
            at("await")) {
            advance();
        } else if (at("new")) {
            advance();
            if (at(".")) {
                // new.target
                advance();
                advance();
                return true;
            }
        } else if (at("yield")) {
            advance();
            if (at(")") || at("]") || at("}") || at(",") || at(";") || at(":") || token_.kind == Token::End || !sameLine()) {
                return true;
            }
            if (at("*")) {
                advance();
            }
        } else {
            return false;
        }
    }
}

/**
 **/
void Eraser::punctuated(const Flags& flags) {
    if (at("(")) {
        if (!arrow(flags)) {
            parenthesised();
        }
    } else if (at("[")) {
        arrayLiteral();
    } else if (at("{")) {
        objectLiteral();
    } else if (at("/") || at("/=")) {
        token_ = scanner_.regex(token_);
        if (token_.kind == Token::Unclosed) {
            refuse("a regular expression here is not closed");
        }
        advance();
    } else if (at("<")) {
        genericArrow(flags);
    } else {
        mismatch();
    }
}

/**
 **/
void Eraser::genericArrow(const Flags& flags) {
    const Mark from = mark();
    try {
        const std::size_t start = token_.begin;
        typeParameters();
        eraseFrom(start);
        if (!at("(") || !arrow(flags)) {
            mismatch();
        }
    } catch (const Mismatch&) {
        back(from);
        refuse("a type assertion written <T>value cannot be erased. Write value as T");
    }
}

/**
 **/
void Eraser::named(const Flags& flags) {
    const Lexeme first = token_;
    const Lexeme next = peek();
    const bool joined = !next.newline;
    if (at("function")) {
        function(false, first.begin);
    } else if (at("class")) {
        classDeclaration();
    } else if (at("async") && joined && next.is("function")) {
        advance();
        function(false, first.begin);
    } else if (at("async") && joined && (next.is("(") || next.is("<") || next.kind == Token::Identifier)) {
        asyncArrow(flags);
    } else if (next.is("=>") && joined) {
        advance();
        advance();
        arrowBody(flags);
    } else {
        advance();
    }
}

/**
 **/
void Eraser::asyncArrow(const Flags& flags) {
    // an async arrow function, or a call of something called async
    const Mark from = mark();
    advance();
    if (at("<")) {
        const std::size_t start = token_.begin;
        try {
            typeParameters();
            eraseFrom(start);
        } catch (const Mismatch&) {
            back(from);
            advance();
            return;
        }
    }
    if (token_.kind == Token::Identifier && peek().is("=>")) {
        advance();
        advance();
        arrowBody(flags);
    } else if (!at("(") || !arrow(flags)) {
        back(from);
        advance();
    }
}

/**
 **/
bool Eraser::operation(const Flags& flags) {
    for (;;) {
        Next next = Next::End;
        switch (token_.kind) {
            case Token::Template:
            case Token::TemplateHead:
                // a tagged template
                templateLiteral(false);
                next = Next::Postfix;
                break;
            case Token::Identifier:
                next = namedOperation(flags);
                break;
            case Token::Punctuator:
                next = punctuatedOperation(flags);
                break;
            case Token::End:
            case Token::Unclosed:
            case Token::Number:
            case Token::String:
            case Token::Private:
            case Token::Regex:
            case Token::TemplateMiddle:
                break;
        }
        if (next != Next::Postfix) {
            return next == Next::Operand;
        }
    }
}

/**
 **/
Eraser::Next Eraser::namedOperation(const Flags& flags) {
    if ((at("as") || at("satisfies")) && sameLine()) {
        const std::size_t start = token_.begin;
        advance();
        if (at("const")) {
            advance();
        } else {
            type();
        }
        eraseFrom(start);
        return Next::Postfix;
    }
    if ((at("in") && flags.in) || at("instanceof")) {
        advance();
        return Next::Operand;
    }
    return Next::End;
}

/**
 **/
Eraser::Next Eraser::punctuatedOperation(const Flags& flags) {
    if (postfix()) {
        return Next::Postfix;
    }
    if (at("<")) {
        if (typeArguments(flags.heritage)) {
            return Next::Postfix;
        }
        advance();
    } else if (at(">")) {
        // > is read alone, so >>, >>> and >= arrive in pieces, which join here
        advance();
        while ((at(">") || at("=")) && token_.begin == previous_.end) {
            advance();
        }
    } else if (at("?")) {
        advance();
        expression({ .comma = false, .returned = false });
        expect(":");
    } else if (at(")") || at("]") || at("}") || at(";") || at(":") || at("{") || at("=>") || at("...") || at("++") || at("--") || at("!") ||
        (at(",") && !flags.comma)) {
        return Next::End;
    } else {
        // a comma, or a binary or assignment operator
        advance();
    }
    return Next::Operand;
}

/**
 **/
bool Eraser::postfix() {
    const bool joined = sameLine();
    if (at("!") && joined) {
        // a non-null assertion
        erase(token_.begin, token_.end);
        advance();
    } else if (at(".") || at("?.")) {
        advance();
        if (at("<") && typeArguments(false)) {
            return true;
        }
        if (token_.kind == Token::Identifier || token_.kind == Token::Private) {
            advance();
        } else if (!at("(") && !at("[")) {
            mismatch();
        }
    } else if (at("(")) {
        arguments();
    } else if (at("[")) {
        advance();
        expression({});
        expect("]");
    } else if ((at("++") || at("--")) && joined) {
        advance();
    } else {
        return false;
    }
    return true;
}

/**
 **/
bool Eraser::arrow(const Flags& flags) {
    const std::pair<std::size_t, bool> here{ token_.begin, flags.returned };
    if (notArrows_.contains(here)) {
        return false;
    }
    const Mark from = mark();
    try {
        parameters(false);
        const bool typed = at(":");
        if (typed) {
            const Lexeme close = previous_;
            const std::size_t colon = token_.begin;
            advance();
            type();
            // => may not follow a line break, so a return type over lines takes the ) with it
            if (source_.substr(close.end, previous_.end - close.end).find('\n') != std::string_view::npos) {
                erase(close.begin, previous_.end, ')');
            } else {
                eraseFrom(colon);
            }
        }
        if (!at("=>") || !sameLine()) {
            mismatch();
        }
        if (typed && !flags.returned) {
            // in a conditional's first branch, an arrow function has a return type only when
            // the : of the conditional follows it
            advance();
            arrowBody(flags);
            if (!at(":")) {
                mismatch();
            }
            return true;
        }
    } catch (const Mismatch&) {
        back(from);
        notArrows_.insert(here);
        return false;
    }
    advance();
    arrowBody(flags);
    return true;
}

/**
 **/
void Eraser::arrowBody(const Flags& flags) {
    if (at("{")) {
        block();
    } else {
        expression({ .comma = false, .in = flags.in, .returned = flags.returned });
    }
}

/**
 **/
void Eraser::parenthesised() {
    expect("(");
    expression({});
    expect(")");
}

/**
 **/
void Eraser::arguments() {
    expect("(");
    while (!at(")")) {
        expression({ .comma = false });
        if (!at(",")) {
            break;
        }
        advance();
    }
    expect(")");
}

/**
 **/
void Eraser::arrayLiteral() {
    expect("[");
    while (!at("]")) {
        if (at(",")) {
            advance();
            continue;
        }
        expression({ .comma = false });
        if (!at(",")) {
            break;
        }
        advance();
    }
    expect("]");
}

/**
 **/
void Eraser::objectLiteral() {
    expect("{");
    while (!at("}")) {
        if (at("...")) {
            advance();
            expression({ .comma = false });
        } else {
            while ((at("get") || at("set") || at("async")) && token_.kind == Token::Identifier) {
                const Lexeme next = peek();
                if (next.kind != Token::Identifier && next.kind != Token::String && next.kind != Token::Number && next.kind != Token::Private &&
                    !next.is("[") && !next.is("*")) {
                    break;
                }
                advance();
            }
            if (at("*")) {
                advance();
            }
            propertyName();
            if (at("(") || at("<")) {
                signature(false);
                block();
            } else if (at(":") || at("=")) {
                advance();
                expression({ .comma = false });
            }
        }
        if (!at(",")) {
            break;
        }
        advance();
    }
    expect("}");
}

/**
 **/
void Eraser::propertyName() {
    if (token_.kind == Token::Identifier || token_.kind == Token::String || token_.kind == Token::Number || token_.kind == Token::Private) {
        advance();
    } else if (at("[")) {
        advance();
        expression({});
        expect("]");
    } else {
        mismatch();
    }
}

/**
 **/
void Eraser::templateLiteral(bool type) {
    if (token_.kind == Token::Template) {
        advance();
        return;
    }
    // a template left open is reported where it opens
    const std::size_t opened = token_.begin;
    for (;;) {
        advance();
        if (type) {
            this->type();
        } else {
            expression({});
        }
        if (!at("}")) {
            mismatch();
        }
        token_ = scanner_.resume(token_);
        if (token_.kind == Token::Unclosed) {
            refuse("a template here is not closed", opened);
        }
        if (token_.kind == Token::Template) {
            advance();
            return;
        }
    }
}

/**
 **/
bool Eraser::typeArguments(bool heritage) {
    const Mark from = mark();
    const std::size_t start = token_.begin;
    try {
        advance();
        typeList();
        expect(">");
    } catch (const Mismatch&) {
        back(from);
        return false;
    }
    // what may follow type arguments in an expression, as TypeScript decides it
    const Lexeme& next = token_;
    bool follows = next.is("(") || next.kind == Token::Template || next.kind == Token::TemplateHead || (heritage && (next.is("{") || next.is("implements")));
    if (!follows && !next.is("<") && !next.is(">") && !next.is("+") && !next.is("-")) {
        follows = next.newline || next.kind == Token::End || next.is(")") || next.is("]") || next.is("}") || next.is(",") || next.is(";") ||
            next.is(".") || next.is("?.") || next.is(":") || next.is("=") || next.is("==") || next.is("===") || next.is("!=") ||
            next.is("!==") || next.is("&&") || next.is("||") || next.is("??") || next.is("?") || next.is("&") || next.is("|") || next.is("^");
    }
    if (!follows) {
        back(from);
        return false;
    }
    eraseFrom(start);
    return true;
}

};  // namespace v3d::script::internal
