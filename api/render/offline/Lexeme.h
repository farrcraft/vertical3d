/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace v3d::render::offline {

/**
 * One lexical unit, whichever language it came from: what kind it is, its text or its value,
 * and where it started. Each language says what its kinds are.
 **/
template <typename Kind>
class Lexeme {
 public:
    Lexeme() = default;
    Lexeme(Kind kind, unsigned int line, unsigned int column) :
        kind_(kind), line_(line), column_(column) {
    }
    Lexeme(Kind kind, const std::string & text, unsigned int line, unsigned int column) :
        kind_(kind), text_(text), line_(line), column_(column) {
    }
    /** A number. Only a number lexeme carries a value. **/
    Lexeme(Kind number, float value, unsigned int line, unsigned int column) :
        kind_(number), value_(value), line_(line), column_(column) {
    }

    Kind kind() const {
        return kind_;
    }

    /**
     * The name of an identifier, the spelling of a symbol, or the contents of a string with
     * its escapes resolved and its quotes gone. Empty for a number.
     **/
    const std::string & text() const {
        return text_;
    }

    float value() const {
        return value_;
    }

    /**
     * Where the lexeme started, counting from one. Parse errors report this position.
     **/
    unsigned int line() const {
        return line_;
    }
    unsigned int column() const {
        return column_;
    }

 private:
    Kind kind_ = Kind::END;
    std::string text_;
    float value_ = 0.0f;
    unsigned int line_ = 0;
    unsigned int column_ = 0;
};

};  // namespace v3d::render::offline
