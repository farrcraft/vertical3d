/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Characters.h>

#include <string>

namespace v3d::render::offline::rib {

/** What kind of thing a Token is. **/
enum class TokenKind {
    END,
    IDENTIFIER,
    STRING,
    NUMBER,
    ARRAY_BEGIN,
    ARRAY_END
};

/**
 * One lexical unit of a RIB stream.
 *
 * An ASCII RIB file holds nothing else: a request name, a quoted string, a number, and
 * the two array delimiters. That is what makes recovery from an unrecognised request
 * possible without knowing its arity - only a string, a number or an array can be an
 * argument, so the next identifier begins the next request whether or not either one is
 * recognised.
 **/
class Token final : public v3d::render::offline::Lexeme<TokenKind> {
 public:
    typedef TokenKind Kind;

    Token() = default;
    Token(Kind kind, unsigned int line, unsigned int column) : Lexeme(kind, line, column) {
    }
    Token(Kind kind, const std::string & text, unsigned int line, unsigned int column) :
        Lexeme(kind, text, line, column) {
    }
    Token(float value, unsigned int line, unsigned int column) : Lexeme(Kind::NUMBER, value, line, column) {
    }
};

};  // namespace v3d::render::offline::rib
