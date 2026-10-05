/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Token.h"

#include <string>

namespace v3d::render::offline::sl {

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

};  // namespace v3d::render::offline::sl
