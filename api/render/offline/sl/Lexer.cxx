/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Lexer.h"

#include <api/render/offline/Characters.h>

#include <algorithm>
#include <cstdlib>
#include <istream>
#include <string>

namespace v3d::render::offline::sl {

namespace {

/**
 * The closed set: the five shader types, the eight data types, the two storage classes,
 * the control flow, and the three lighting constructs. A name outside it is an identifier,
 * so a shader may declare a variable called "output" or "noise" and have it mean what it
 * declared.
 **/
const char* const KEYWORDS[] = {
    "surface", "light", "displacement", "volume", "imager",
    "float", "point", "vector", "normal", "color", "matrix", "string", "void",
    "uniform", "varying",
    "if", "else", "for", "while", "break", "continue", "return",
    "illuminance", "illuminate", "solar"
};

bool punctuation(int c) {
    return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == ',' || c == ';';
}

bool keyword(const std::string & name) {
    return std::ranges::any_of(KEYWORDS, [&name](const char* word) { return name == word; });
}

};  // namespace

Lexer::Lexer(std::istream & stream) : in_(stream) {
}

bool Lexer::skipSpace() {
    for (;;) {
        const int c = in_.look();
        if (Characters::space(c)) {
            in_.get();
            continue;
        }
        if (c != '/') {
            return true;
        }
        // a '/' is a division until the character after it says otherwise, and reading that
        // character means the '/' has already been taken
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
    return Token(Token::Kind::END, line, column);
}

Token Lexer::scanString(unsigned int line, unsigned int column) {
    in_.get();  // the opening quote
    std::string text;
    if (!in_.quoted(true, &text)) {
        return fail("unterminated string starting", line, column);
    }
    return Token(Token::Kind::STRING, text, line, column);
}

Token Lexer::scanNumber(unsigned int line, unsigned int column) {
    // no sign is taken here, unlike RIB's numbers: a '-' in SL is the subtraction or the
    // negation, so "a-1" is three tokens rather than two
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
    return Token(std::strtof(text.c_str(), nullptr), line, column);
}

Token Lexer::scanName(unsigned int line, unsigned int column) {
    std::string text;
    while (Characters::alpha(in_.look()) || Characters::digit(in_.look())) {
        text += static_cast<char>(in_.get());
    }
    const Token::Kind kind = keyword(text) ? Token::Kind::KEYWORD : Token::Kind::IDENTIFIER;
    return Token(kind, text, line, column);
}

Token Lexer::scanOperator(unsigned int line, unsigned int column) {
    const int first = in_.get();
    std::string text(1, static_cast<char>(first));

    // the two character forms, longest match first: a '<' is not a '<=' until the '=' is
    // seen, and a '&' that is not half of a '&&' is not a token in this language
    const int second = in_.look();
    const bool assigns = (first == '+' || first == '-' || first == '*' || first == '/' ||
        first == '=' || first == '!' || first == '<' || first == '>') && second == '=';
    const bool doubled = (first == '&' || first == '|') && second == first;
    if (assigns || doubled) {
        text += static_cast<char>(in_.get());
        return Token(Token::Kind::OPERATOR, text, line, column);
    }
    if (first == '&' || first == '|') {
        return fail("'" + text + "' is not an operator - '" + text + text + "' is", line, column);
    }
    return Token(Token::Kind::OPERATOR, text, line, column);
}

Token Lexer::scan() {
    if (!error_.empty()) {
        return Token(Token::Kind::END, in_.line(), in_.column());
    }
    if (!skipSpace()) {
        return Token(Token::Kind::END, in_.line(), in_.column());
    }

    const unsigned int line = in_.line();
    const unsigned int column = in_.column() + 1;
    const int c = in_.look();
    if (c == Characters::END) {
        return Token(Token::Kind::END, line, column);
    }
    if (c == '#') {
        // '#' has no other meaning in SL, so wherever one is, it came from a directive this
        // lexer will not resolve. Reporting it by name says the shader needs a preprocessor,
        // rather than suggesting that it is misspelt
        in_.get();
        return fail("the C preprocessor is not run, so a '#' directive cannot be read", line, column);
    }
    if (c == '"') {
        return scanString(line, column);
    }
    if (Characters::digit(c)) {
        return scanNumber(line, column);
    }
    if (c == '.') {
        // a '.' opens a number when a digit follows it and is the dot product otherwise
        in_.get();
        if (Characters::digit(in_.look())) {
            in_.unget('.');
            return scanNumber(line, column);
        }
        return Token(Token::Kind::OPERATOR, ".", line, column);
    }
    if (Characters::alpha(c)) {
        return scanName(line, column);
    }
    if (punctuation(c)) {
        in_.get();
        return Token(Token::Kind::PUNCTUATION, std::string(1, static_cast<char>(c)), line, column);
    }
    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == '=' || c == '!' ||
        c == '<' || c == '>' || c == '&' || c == '|' || c == '?' || c == ':') {
        return scanOperator(line, column);
    }
    in_.get();
    return fail(std::string("unexpected character '") + static_cast<char>(c) + "'", line, column);
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
