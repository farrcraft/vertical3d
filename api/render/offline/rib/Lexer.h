/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <iosfwd>
#include <string>

#include "Token.h"

namespace v3d::render::offline::rib {

/**
 * Turns a RIB stream into tokens.
 *
 * Whitespace and newlines are not significant - an array spans lines in real RIB - so the
 * token and not the line is the unit above this. Comments run from '#' to the end of the
 * line; '##' is a structure comment carrying metadata and is skipped the same way.
 *
 * A number token holds a float and the caller narrows on use: "Format 640 480 1" and
 * "Clipping 10 1000.0" are the same kind, and an integer conversion over the second one
 * throws.
 **/
class Lexer final {
 public:
    /**
     * Binary and gzipped RIB are detected here, by the bytes the stream opens with, and
     * leave error() set with next() at END. Both parse into nonsense otherwise.
     *
     * The stream is read from as tokens are asked for and has to outlive the lexer.
     **/
    explicit Lexer(std::istream & stream);

    /**
     * The next token, consuming it. END once the stream is spent or an error is set.
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

    /**
     * Consume one character, tracking the position.
     **/
    int get();
    int look();
    void skipSpace();
    Token fail(const std::string & message, unsigned int line, unsigned int column);

    std::istream & stream_;
    Token peeked_;
    bool peeking_ = false;
    std::string error_;
    unsigned int line_ = 1;
    unsigned int column_ = 0;
};

};  // namespace v3d::render::offline::rib
