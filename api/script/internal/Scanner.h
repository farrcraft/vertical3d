/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>
#include <string_view>

#include "Lexeme.h"

namespace v3d::script::internal {

/**
 * Splits TypeScript into tokens, one at a time, where its parser asks.
 *
 * A slash is always division, and a > is always one character, as TypeScript's own scanner
 * reads them. The parser asks for a slash to be read again as a regular expression where it
 * expects an operand, and joins the two tokens of a >> or a >=. A } that closes a template's
 * substitution is read again as the rest of the template when the parser reaches it.
 **/
class Scanner final {
 public:
    explicit Scanner(std::string_view source);

    /**
     * @return the next token after the one last read
     **/
    Lexeme next();

    /**
     * @return a slash token read again as a regular expression
     **/
    Lexeme regex(const Lexeme& slash);

    /**
     * @return a } token read again as the rest of the template it closes a substitution of
     **/
    Lexeme resume(const Lexeme& brace);

    /**
     * Where the next token is read from, so that the parser can go back to a token it read.
     **/
    std::size_t position() const noexcept;
    void position(std::size_t offset) noexcept;

 private:
    /**
     * Skip white space and comments.
     * @return whether a line break was among them, or nothing when a comment never ends
     **/
    bool skip(bool* unclosed);

    /**
     * Read from a template's opening ` or a substitution's } to the ${ or ` that ends the piece.
     **/
    Lexeme templated(std::size_t begin, bool head, bool newline);

    /**
     * Read a string from its opening quote.
     **/
    Lexeme quoted(std::size_t begin, bool newline);

    Lexeme made(Token kind, std::size_t begin, bool newline) const;
    std::size_t name(std::size_t at) const;
    std::size_t number(std::size_t at) const;
    std::size_t punctuator(std::size_t at) const;
    char at(std::size_t offset) const noexcept;

    std::string_view source_;
    std::size_t position_ = 0;
};

};  // namespace v3d::script::internal
