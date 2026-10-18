/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Lexer.h"

#include <api/render/offline/Characters.h>

#include "Token.h"

#include <algorithm>
#include <cstdlib>
#include <istream>
#include <string>

namespace v3d::render::offline::sl {

namespace {

/**
 * The keywords: the shader types, the data types, the storage classes, the control flow,
 * and the lighting constructs. Any other name is an identifier, so a shader may declare a
 * variable called "output" or "noise".
 **/
const char* const KEYWORDS[] = {
    "surface", "light", "displacement", "volume", "imager",
    "float", "point", "vector", "normal", "color", "matrix", "string", "void",
    "uniform", "varying",
    "if", "else", "for", "while", "break", "continue", "return",
    "illuminance", "illuminate", "solar"
};

bool punctuation(int character) {
    return character == '(' || character == ')' || character == '[' || character == ']' || character == '{' || character == '}' || character == ',' || character == ';';
}

bool keyword(const std::string & name) {
    return std::ranges::any_of(KEYWORDS, [&name](const char* word) { return name == word; });
}

};  // namespace

Lexer::Lexer(std::istream & stream) : in_(stream) {
}

bool Lexer::skipSpace() {
    for (;;) {
        const int character = in_.look();
        if (Characters::space(character)) {
            in_.get();
            continue;
        }
        if (character != '/') {
            return true;
        }
        // a '/' is a division unless the next character starts a comment. Reading that
        // character takes the '/' first
        const unsigned int line = in_.line();
        const unsigned int column = in_.column() + 1;
        in_.get();
        const int second = in_.look();
        if (second == '/') {
            while (in_.look() != Characters::END && in_.look() != '\n') {
                in_.get();
            }
            continue;
        }
        if (second != '*') {
            in_.unget('/');
            return true;
        }
        in_.get();
        // a block comment does not nest: the first close ends it, and a "//" inside one is
        // as much a part of the comment as anything else
        for (;;) {
            const int inside = in_.get();
            if (inside == Characters::END) {
                error_ = "unterminated block comment starting" + Characters::position(line, column);
                return false;
            }
            if (inside == '*' && in_.look() == '/') {
                in_.get();
                break;
            }
        }
    }
}

Token Lexer::fail(const std::string & message, unsigned int line, unsigned int column) {
    if (error_.empty()) {
        error_ = message + Characters::position(line, column);
    }
    return Token(Token::Kind::END, {.line = line, .column = column});
}

Token Lexer::scanString(unsigned int line, unsigned int column) {
    in_.get();  // the opening quote
    std::string text;
    if (!in_.quoted(true, &text)) {
        return fail("unterminated string starting", line, column);
    }
    return Token(Token::Kind::STRING, text, {.line = line, .column = column});
}

Token Lexer::scanNumber(unsigned int line, unsigned int column) {
    // unlike a RIB number, an SL number takes no sign. A '-' in SL is a subtraction or a
    // negation, so "a-1" is three tokens
    std::string text;
    bool digits = false;
    while (Characters::digit(in_.look())) {
        text += static_cast<char>(in_.get());
        digits = true;
    }
    if (in_.look() == '.') {
        text += static_cast<char>(in_.get());
        while (Characters::digit(in_.look())) {
            text += static_cast<char>(in_.get());
            digits = true;
        }
    }
    if (digits && (in_.look() == 'e' || in_.look() == 'E')) {
        text += static_cast<char>(in_.get());
        if (in_.look() == '+' || in_.look() == '-') {
            text += static_cast<char>(in_.get());
        }
        while (Characters::digit(in_.look())) {
            text += static_cast<char>(in_.get());
        }
    }
    if (!digits) {
        return fail("'" + text + "' is not a number", line, column);
    }
    return Token(std::strtof(text.c_str(), nullptr), {.line = line, .column = column});
}

Token Lexer::scanName(unsigned int line, unsigned int column) {
    std::string text;
    while (Characters::alpha(in_.look()) || Characters::digit(in_.look())) {
        text += static_cast<char>(in_.get());
    }
    const Token::Kind kind = keyword(text) ? Token::Kind::KEYWORD : Token::Kind::IDENTIFIER;
    return Token(kind, text, {.line = line, .column = column});
}

Token Lexer::scanOperator(unsigned int line, unsigned int column) {
    const int first = in_.get();
    std::string text(1, static_cast<char>(first));

    // the two-character forms, longest match first. A '<' followed by '=' is '<='. A '&' or
    // '|' that is not doubled is not a token in SL
    const int second = in_.look();
    const bool assigns = (first == '+' || first == '-' || first == '*' || first == '/' ||
        first == '=' || first == '!' || first == '<' || first == '>') && second == '=';
    const bool doubled = (first == '&' || first == '|') && second == first;
    if (assigns || doubled) {
        text += static_cast<char>(in_.get());
        return Token(Token::Kind::OPERATOR, text, {.line = line, .column = column});
    }
    if (first == '&' || first == '|') {
        return fail("'" + text + "' is not an operator - '" + text + text + "' is", line, column);
    }
    return Token(Token::Kind::OPERATOR, text, {.line = line, .column = column});
}

Token Lexer::scan() {
    if (!error_.empty()) {
        return Token(Token::Kind::END, {.line = in_.line(), .column = in_.column()});
    }
    if (!skipSpace()) {
        return Token(Token::Kind::END, {.line = in_.line(), .column = in_.column()});
    }

    const unsigned int line = in_.line();
    const unsigned int column = in_.column() + 1;
    const int character = in_.look();
    if (character == Characters::END) {
        return Token(Token::Kind::END, {.line = line, .column = column});
    }
    if (character == '#') {
        // '#' has no meaning in SL except as a preprocessor directive, which this lexer does
        // not resolve. The error says the shader needs a preprocessor, so it does not read
        // as a misspelling
        in_.get();
        return fail("the C preprocessor is not run, so a '#' directive cannot be read", line, column);
    }
    if (character == '"') {
        return scanString(line, column);
    }
    if (Characters::digit(character)) {
        return scanNumber(line, column);
    }
    if (character == '.') {
        // a '.' opens a number when a digit follows it and is the dot product otherwise
        in_.get();
        if (Characters::digit(in_.look())) {
            in_.unget('.');
            return scanNumber(line, column);
        }
        return Token(Token::Kind::OPERATOR, ".", {.line = line, .column = column});
    }
    if (Characters::alpha(character)) {
        return scanName(line, column);
    }
    if (punctuation(character)) {
        in_.get();
        return Token(Token::Kind::PUNCTUATION, std::string(1, static_cast<char>(character)), {.line = line, .column = column});
    }
    if (character == '+' || character == '-' || character == '*' || character == '/' || character == '^' || character == '=' || character == '!' ||
        character == '<' || character == '>' || character == '&' || character == '|' || character == '?' || character == ':') {
        return scanOperator(line, column);
    }
    in_.get();
    return fail(std::string("unexpected character '") + static_cast<char>(character) + "'", line, column);
}

Token Lexer::next() {
    if (peeking_) {
        peeking_ = false;
        return peeked_;
    }
    return scan();
}

const Token & Lexer::peek() {
    if (!peeking_) {
        peeked_ = scan();
        peeking_ = true;
    }
    return peeked_;
}

const std::string & Lexer::error() const {
    return error_;
}

};  // namespace v3d::render::offline::sl
