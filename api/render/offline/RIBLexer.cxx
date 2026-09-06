/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RIBLexer.h"

#include <cstdlib>
#include <istream>
#include <string>

namespace v3d::render::offline {

namespace {

const int END_OF_STREAM = -1;

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

std::string position(unsigned int line, unsigned int column) {
    return " at line " + std::to_string(line) + ", column " + std::to_string(column);
}

};  // namespace

RIBToken::RIBToken() {
}

RIBToken::RIBToken(Kind kind, unsigned int line, unsigned int column) :
    kind_(kind),
    line_(line),
    column_(column) {
}

RIBToken::RIBToken(Kind kind, const std::string & text, unsigned int line, unsigned int column) :
    kind_(kind),
    text_(text),
    line_(line),
    column_(column) {
}

RIBToken::RIBToken(float value, unsigned int line, unsigned int column) :
    kind_(Kind::NUMBER),
    value_(value),
    line_(line),
    column_(column) {
}

RIBToken::Kind RIBToken::kind() const {
    return kind_;
}

const std::string & RIBToken::text() const {
    return text_;
}

float RIBToken::value() const {
    return value_;
}

unsigned int RIBToken::line() const {
    return line_;
}

unsigned int RIBToken::column() const {
    return column_;
}

RIBLexer::RIBLexer(std::istream & stream) : stream_(stream) {
    char header[2] = { 0, 0 };
    stream_.read(header, 2);
    const std::streamsize count = stream_.gcount();
    stream_.clear();
    stream_.seekg(0);

    const unsigned char first = static_cast<unsigned char>(header[0]);
    const unsigned char second = static_cast<unsigned char>(header[1]);
    if (count >= 2 && first == 0x1f && second == 0x8b) {
        error_ = "gzipped RIB is not supported - the stream opens with a gzip header";
    } else if (count >= 1 && first >= 0x80) {
        error_ = "binary RIB is not supported - the stream opens with an encoded request";
    }
}

int RIBLexer::get() {
    const int c = stream_.get();
    if (c == '\n') {
        line_++;
        column_ = 0;
    } else if (c != END_OF_STREAM) {
        column_++;
    }
    return c;
}

int RIBLexer::look() {
    const int c = stream_.peek();
    return stream_.good() ? c : END_OF_STREAM;
}

void RIBLexer::skipSpace() {
    for (;;) {
        const int c = look();
        if (space(c)) {
            get();
        } else if (c == '#') {
            while (look() != END_OF_STREAM && look() != '\n') {
                get();
            }
        } else {
            return;
        }
    }
}

RIBToken RIBLexer::fail(const std::string & message, unsigned int line, unsigned int column) {
    if (error_.empty()) {
        error_ = message + position(line, column);
    }
    return RIBToken(RIBToken::Kind::END, line, column);
}

RIBToken RIBLexer::scanString(unsigned int line, unsigned int column) {
    get();  // the opening quote
    std::string text;
    for (;;) {
        const int c = get();
        if (c == END_OF_STREAM) {
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
    return RIBToken(RIBToken::Kind::STRING, text, line, column);
}

RIBToken RIBLexer::scanNumber(unsigned int line, unsigned int column) {
    std::string text;
    bool digits = false;
    if (look() == '+' || look() == '-') {
        text += static_cast<char>(get());
    }
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
    return RIBToken(std::strtof(text.c_str(), nullptr), line, column);
}

RIBToken RIBLexer::scanIdentifier(unsigned int line, unsigned int column) {
    std::string text;
    while (alpha(look()) || digit(look())) {
        text += static_cast<char>(get());
    }
    return RIBToken(RIBToken::Kind::IDENTIFIER, text, line, column);
}

RIBToken RIBLexer::scan() {
    if (!error_.empty()) {
        return RIBToken(RIBToken::Kind::END, line_, column_);
    }
    skipSpace();

    const unsigned int line = line_;
    const unsigned int column = column_ + 1;
    const int c = look();
    if (c == END_OF_STREAM) {
        return RIBToken(RIBToken::Kind::END, line, column);
    }
    if (c == '[') {
        get();
        return RIBToken(RIBToken::Kind::ARRAY_BEGIN, line, column);
    }
    if (c == ']') {
        get();
        return RIBToken(RIBToken::Kind::ARRAY_END, line, column);
    }
    if (c == '"') {
        return scanString(line, column);
    }
    if (c == '+' || c == '-' || c == '.' || digit(c)) {
        return scanNumber(line, column);
    }
    if (alpha(c)) {
        return scanIdentifier(line, column);
    }
    get();
    return fail(std::string("unexpected character '") + static_cast<char>(c) + "'", line, column);
}

RIBToken RIBLexer::next() {
    if (peeking_) {
        peeking_ = false;
        return peeked_;
    }
    return scan();
}

const RIBToken & RIBLexer::peek() {
    if (!peeking_) {
        peeked_ = scan();
        peeking_ = true;
    }
    return peeked_;
}

const std::string & RIBLexer::error() const {
    return error_;
}

};  // namespace v3d::render::offline
