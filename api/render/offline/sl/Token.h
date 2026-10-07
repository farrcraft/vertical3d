/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Lexeme.h>

#include <string>

namespace v3d::render::offline::sl {

/** What kind of thing a Token is. **/
enum class TokenKind {
    END,
    IDENTIFIER,
    KEYWORD,
    NUMBER,
    STRING,
    /**
     * Something that combines values: arithmetic, comparison, logic, assignment, the
     * two halves of the ternary, and SL's '.' for a dot product and '^' for a cross.
     **/
    OPERATOR,
    /**
     * Something that groups or separates: the three bracket pairs, a comma, a
     * semicolon.
     **/
    PUNCTUATION
};

/**
 * One lexical unit of a shading language source.
 *
 * A keyword is separated from an identifier here rather than in the parser because the
 * set is closed and small. It holds the shader types, the data types, the storage classes,
 * the control flow, and the three lighting constructs. Everything else that looks like a
 * name is an identifier. The symbol table decides what an identifier means, not the lexer,
 * so a shader can declare a variable named after a built-in.
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

};  // namespace v3d::render::offline::sl
