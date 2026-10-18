/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Characters.h>

#include <iosfwd>
#include <string>

#include "Token.h"

namespace v3d::render::offline::rib {

/**
 * Turns a RIB stream into tokens.
 *
 * Whitespace and newlines are not significant, because an array can span lines in RIB. The
 * reader therefore works on tokens, not lines. A comment runs from '#' to the end of the
 * line. A '##' structure comment carries metadata and is skipped the same way.
 *
 * A number token holds a float, and the caller converts it where it needs an integer:
 * "Format 640 480 1" and "Clipping 10 1000.0" produce the same kind of token. Where a request
 * takes an integer, the reader checks the range before converting and truncates a fraction.
 * A count or a handle out of range fails the stream. A picture size or frame number out of
 * range skips the request.
 **/
class Lexer final {
 public:
    /**
     * Detects binary and gzipped RIB from the stream's opening bytes, which this lexer cannot
     * read. Either one sets error(), and next() then returns END. A UTF-8 byte order mark is
     * skipped.
     *
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
    Token scanIdentifier(unsigned int line, unsigned int column);

    void skipSpace();
    Token fail(const std::string & message, unsigned int line, unsigned int column);

    v3d::render::offline::Characters in_;
    Token peeked_;
    bool peeking_ = false;
    std::string error_;
};

};  // namespace v3d::render::offline::rib
