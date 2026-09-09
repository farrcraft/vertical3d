/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Lexer.h"

#include <algorithm>
#include <cstdlib>
#include <istream>
#include <string>

namespace v3d::render::offline::sl {

namespace {

const int END_OF_STREAM = -1;

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

bool digit(int c) {
    return c >= '0' && c <= '9';
}

bool octal(int c) {
    return c >= '0' && c <= '7';
}

bool alpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool space(int c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

bool punctuation(int c) {
    return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == ',' || c == ';';
}

bool keyword(const std::string & name) {
    return std::ranges::any_of(KEYWORDS, [&name](const char* word) { return name == word; });
}

std::string position(unsigned int line, unsigned int column) {
    return " at line " + std::to_string(line) + ", column " + std::to_string(column);
}

};  // namespace

Token::Token() {
}

Token::Token(Kind kind, unsigned int line, unsigned int column) :
    kind_(kind),
    line_(line),
    column_(column) {
}

Token::Token(Kind kind, const std::string & text, unsigned int line, unsigned int column) :
    kind_(kind),
    text_(text),
    line_(line),
    column_(column) {
}

Token::Token(float value, unsigned int line, unsigned int column) :
    kind_(Kind::NUMBER),
    value_(value),
    line_(line),
    column_(column) {
}

Token::Kind Token::kind() const {
    return kind_;
}

const std::string & Token::text() const {
    return text_;
}

float Token::value() const {
    return value_;
}

unsigned int Token::line() const {
    return line_;
}

unsigned int Token::column() const {
    return column_;
}

Lexer::Lexer(std::istream & stream) : stream_(stream) {
}

int Lexer::get() {
    int c = END_OF_STREAM;
    if (pushed_) {
        c = pushback_;
        pushed_ = false;
    } else {
        c = stream_.get();
    }
    if (c == '\n') {
        line_++;
        column_ = 0;
    } else if (c != END_OF_STREAM) {
        column_++;
    }
    return c;
}

int Lexer::look() {
    if (pushed_) {
        return pushback_;
    }
    const int c = stream_.peek();
    return stream_.good() ? c : END_OF_STREAM;
}

void Lexer::unget(int c) {
    pushback_ = c;
    pushed_ = true;
    if (c != END_OF_STREAM) {
        column_--;
    }
}

bool Lexer::skipSpace() {
    for (;;) {
        const int c = look();
        if (space(c)) {
            get();
            continue;
        }
        if (c != '/') {
            return true;
        }
        // a '/' is a division until the character after it says otherwise, and reading that
        // character means the '/' has already been taken
        const unsigned int line = line_;
        const unsigned int column = column_ + 1;
        get();
        const int second = look();
        if (second == '/') {
            while (look() != END_OF_STREAM && look() != '\n') {
                get();
            }
            continue;
        }
        if (second != '*') {
            unget('/');
            return true;
        }
        get();
        // a block comment does not nest: the first close ends it, and a "//" inside one is
        // as much a part of the comment as anything else
        for (;;) {
            const int inside = get();
            if (inside == END_OF_STREAM) {
                error_ = "unterminated block comment starting" + position(line, column);
                return false;
            }
            if (inside == '*' && look() == '/') {
                get();
                break;
            }
        }
    }
}

Token Lexer::fail(const std::string & message, unsigned int line, unsigned int column) {
    if (error_.empty()) {
        error_ = message + position(line, column);
    }
    return Token(Token::Kind::END, line, column);
}

Token Lexer::scanString(unsigned int line, unsigned int column) {
    get();  // the opening quote
    std::string text;
    for (;;) {
        const int c = get();
        // a string does not span a line: a missing close quote otherwise swallows the rest
        // of the shader and reports the error wherever the next quote happens to be
        if (c == END_OF_STREAM || c == '\n') {
            return fail("unterminated string starting", line, column);
        }
        if (c == '"') {
            break;
        }
        if (c != '\\') {
            text += static_cast<char>(c);
            continue;
        }
        const int escape = get();
        switch (escape) {
            case 'n': text += '\n'; break;
            case 't': text += '\t'; break;
            case 'r': text += '\r'; break;
            case 'b': text += '\b'; break;
            case 'f': text += '\f'; break;
            case '\\': text += '\\'; break;
            case '"': text += '"'; break;
            case END_OF_STREAM:
                return fail("unterminated string starting", line, column);
            default:
                if (octal(escape)) {
                    int value = escape - '0';
                    for (int i = 1; i < 3 && octal(look()); i++) {
                        value = value * 8 + (get() - '0');
                    }
                    text += static_cast<char>(value);
                } else {
                    // an escape the standard does not define is the character itself
                    text += static_cast<char>(escape);
                }
                break;
        }
    }
    return Token(Token::Kind::STRING, text, line, column);
}

Token Lexer::scanNumber(unsigned int line, unsigned int column) {
    // no sign is taken here, unlike RIB's numbers: a '-' in SL is the subtraction or the
    // negation, so "a-1" is three tokens rather than two
    std::string text;
    bool digits = false;
    while (digit(look())) {
        text += static_cast<char>(get());
        digits = true;
    }
    if (look() == '.') {
        text += static_cast<char>(get());
        while (digit(look())) {
            text += static_cast<char>(get());
            digits = true;
        }
    }
    if (digits && (look() == 'e' || look() == 'E')) {
        text += static_cast<char>(get());
        if (look() == '+' || look() == '-') {
            text += static_cast<char>(get());
        }
        while (digit(look())) {
            text += static_cast<char>(get());
        }
    }
    if (!digits) {
        return fail("'" + text + "' is not a number", line, column);
    }
    return Token(std::strtof(text.c_str(), nullptr), line, column);
}

Token Lexer::scanName(unsigned int line, unsigned int column) {
    std::string text;
    while (alpha(look()) || digit(look())) {
        text += static_cast<char>(get());
    }
    const Token::Kind kind = keyword(text) ? Token::Kind::KEYWORD : Token::Kind::IDENTIFIER;
    return Token(kind, text, line, column);
}

Token Lexer::scanOperator(unsigned int line, unsigned int column) {
    const int first = get();
    std::string text(1, static_cast<char>(first));

    // the two character forms, longest match first: a '<' is not a '<=' until the '=' is
    // seen, and a '&' that is not half of a '&&' is nothing in this language
    const int second = look();
    const bool assigns = (first == '+' || first == '-' || first == '*' || first == '/' ||
        first == '=' || first == '!' || first == '<' || first == '>') && second == '=';
    const bool doubled = (first == '&' || first == '|') && second == first;
    if (assigns || doubled) {
        text += static_cast<char>(get());
        return Token(Token::Kind::OPERATOR, text, line, column);
    }
    if (first == '&' || first == '|') {
        return fail("'" + text + "' is not an operator - '" + text + text + "' is", line, column);
    }
    return Token(Token::Kind::OPERATOR, text, line, column);
}

Token Lexer::scan() {
    if (!error_.empty()) {
        return Token(Token::Kind::END, line_, column_);
    }
    if (!skipSpace()) {
        return Token(Token::Kind::END, line_, column_);
    }

    const unsigned int line = line_;
    const unsigned int column = column_ + 1;
    const int c = look();
    if (c == END_OF_STREAM) {
        return Token(Token::Kind::END, line, column);
    }
    if (c == '#') {
        // '#' has no other meaning in SL, so wherever one is, it came from a directive this
        // lexer will not resolve. Reporting it by name is the difference between a shader
        // that needs a tool this tree does not have and a shader that is misspelt
        get();
        return fail("the C preprocessor is not run, so a '#' directive cannot be read", line, column);
    }
    if (c == '"') {
        return scanString(line, column);
    }
    if (digit(c)) {
        return scanNumber(line, column);
    }
    if (c == '.') {
        // a '.' opens a number when a digit follows it and is the dot product otherwise
        get();
        if (digit(look())) {
            unget('.');
            return scanNumber(line, column);
        }
        return Token(Token::Kind::OPERATOR, ".", line, column);
    }
    if (alpha(c)) {
        return scanName(line, column);
    }
    if (punctuation(c)) {
        get();
        return Token(Token::Kind::PUNCTUATION, std::string(1, static_cast<char>(c)), line, column);
    }
    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == '=' || c == '!' ||
        c == '<' || c == '>' || c == '&' || c == '|' || c == '?' || c == ':') {
        return scanOperator(line, column);
    }
    get();
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
