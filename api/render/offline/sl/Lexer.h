/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

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
    Token scanName(unsigned int line, unsigned int column);
    Token scanOperator(unsigned int line, unsigned int column);

    /**
     * Consume one character, tracking the position.
     **/
    int get();
    int look();
    /**
     * Put back the character get() just returned, one deep.
     *
     * Deciding whether a '/' opens a comment or is a division takes the character after
     * it, and a '.' is a number or a dot product on the same terms. The stream's own
     * putback cannot serve: it fails once the stream has hit its end, which is the case a
     * trailing '/' produces.
     **/
    void unget(int c);
    /**
     * Whitespace and both comment forms, which stand in the same places.
     *
     * @return false when a comment was left unterminated, which sets the error
     **/
    bool skipSpace();
    Token fail(const std::string & message, unsigned int line, unsigned int column);

    std::istream & stream_;
    Token peeked_;
    bool peeking_ = false;
    int pushback_ = 0;
    bool pushed_ = false;
    std::string error_;
    unsigned int line_ = 1;
    unsigned int column_ = 0;
};

};  // namespace v3d::render::offline::sl
