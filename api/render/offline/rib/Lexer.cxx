/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Lexer.h"

#include <api/render/offline/Characters.h>

#include <cstdlib>
#include <istream>
#include <string>

namespace v3d::render::offline::rib {

Lexer::Lexer(std::istream & stream) : in_(stream) {
    char header[2] = { 0, 0 };
    stream.read(header, 2);
    const std::streamsize count = stream.gcount();
    stream.clear();
    stream.seekg(0);

    const unsigned char first = static_cast<unsigned char>(header[0]);
    const unsigned char second = static_cast<unsigned char>(header[1]);
    if (count >= 2 && first == 0x1f && second == 0x8b) {
        error_ = "gzipped RIB is not supported - the stream opens with a gzip header";
    } else if (count >= 1 && first >= 0x80) {
        error_ = "binary RIB is not supported - the stream opens with an encoded request";
    }
}

void Lexer::skipSpace() {
    for (;;) {
        const int c = in_.look();
        if (Characters::space(c)) {
            in_.get();
        } else if (c == '#') {
            while (in_.look() != Characters::END && in_.look() != '\n') {
                in_.get();
            }
        } else {
            return;
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
    if (!in_.quoted(false, &text)) {
        return fail("unterminated string starting", line, column);
    }
    return Token(Token::Kind::STRING, text, line, column);
}

Token Lexer::scanNumber(unsigned int line, unsigned int column) {
    std::string text;
    bool digits = false;
    if (in_.look() == '+' || in_.look() == '-') {
        text += static_cast<char>(in_.get());
    }
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

Token Lexer::scanIdentifier(unsigned int line, unsigned int column) {
    std::string text;
    while (Characters::alpha(in_.look()) || Characters::digit(in_.look())) {
        text += static_cast<char>(in_.get());
    }
    return Token(Token::Kind::IDENTIFIER, text, line, column);
}

Token Lexer::scan() {
    if (!error_.empty()) {
        return Token(Token::Kind::END, in_.line(), in_.column());
    }
    skipSpace();

    const unsigned int line = in_.line();
    const unsigned int column = in_.column() + 1;
    const int c = in_.look();
    if (c == Characters::END) {
        return Token(Token::Kind::END, line, column);
    }
    if (c == '[') {
        in_.get();
        return Token(Token::Kind::ARRAY_BEGIN, line, column);
    }
    if (c == ']') {
        in_.get();
        return Token(Token::Kind::ARRAY_END, line, column);
    }
    if (c == '"') {
        return scanString(line, column);
    }
    if (c == '+' || c == '-' || c == '.' || Characters::digit(c)) {
        return scanNumber(line, column);
    }
    if (Characters::alpha(c)) {
        return scanIdentifier(line, column);
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

};  // namespace v3d::render::offline::rib
