/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Lexeme.h>

#include <cstdint>
#include <string>

namespace v3d::render::offline::rib {

/** What kind of thing a Token is. **/
enum class TokenKind : std::uint8_t {
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
 * An ASCII RIB file holds only these: a request name, a quoted string, a number, and the
 * two array delimiters. A request name is the only identifier, so the reader can skip an
 * unrecognised request without knowing how many arguments it takes.
 **/
class Token final : public v3d::render::offline::Lexeme<TokenKind> {
 public:
    typedef TokenKind Kind;

    Token() = default;
    Token(Kind kind, const v3d::render::offline::Position & at) : Lexeme(kind, at) {
    }
    Token(Kind kind, const std::string & text, const v3d::render::offline::Position & at) :
        Lexeme(kind, text, at) {
    }
    Token(float value, const v3d::render::offline::Position & at) : Lexeme(Kind::NUMBER, value, at) {
    }
};

};  // namespace v3d::render::offline::rib
