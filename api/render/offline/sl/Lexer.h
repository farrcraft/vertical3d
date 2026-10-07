/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Characters.h>

#include <iosfwd>
#include <string>

#include "Token.h"

namespace v3d::render::offline::sl {

/**
 * Turns a shading language source into tokens.
 *
 * Whitespace and newlines are not significant. Comments are C's, both forms, and a block
 * comment does not nest: the first close ends it however many opens came before.
 *
 * The C preprocessor is **not run**. Real .sl files are put through cpp for #include and
 * #define, and a shader that needs one is rejected by name here rather than mis-parsed
 * further on: a '#' is a diagnostic, not a comment.
 **/
class Lexer final {
 public:
    /**
     * The stream is read as tokens are requested, and must outlive the lexer.
     **/
    explicit Lexer(std::istream & stream);

    /**
     * The next token, consuming it. END once the stream is exhausted or an error is set.
     **/
    Token next();

    /**
     * The next token, leaving it for next() to return.
     **/
    const Token & peek();

    /**
     * What went wrong, or empty. A lexer with an error set yields nothing but END.
     **/
    const std::string & error() const;

 private:
    Token scan();
    Token scanString(unsigned int line, unsigned int column);
    Token scanNumber(unsigned int line, unsigned int column);
    Token scanName(unsigned int line, unsigned int column);
    Token scanOperator(unsigned int line, unsigned int column);

    /**
     * Whitespace and both comment forms, which may appear in the same places.
     *
     * @return false when a comment was left unterminated, which sets the error
     **/
    bool skipSpace();
    Token fail(const std::string & message, unsigned int line, unsigned int column);

    v3d::render::offline::Characters in_;
    Token peeked_;
    bool peeking_ = false;
    std::string error_;
};

};  // namespace v3d::render::offline::sl
