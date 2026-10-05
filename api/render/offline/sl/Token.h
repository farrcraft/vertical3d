/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace v3d::render::offline::sl {

/**
 * One lexical unit of a shading language source.
 *
 * A keyword is separated from an identifier here rather than in the parser because the
 * set is closed and small: the shader types, the data types, the storage classes, the
 * control flow, and the three lighting constructs. Everything else that looks like a name
 * is an identifier, and what it means is the symbol table's answer rather than the
 * lexer's - which is what lets a shader declare a variable named after a built-in.
 **/
class Token final {
 public:
    enum class Kind {
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

    Token();
    Token(Kind kind, unsigned int line, unsigned int column);
    Token(Kind kind, const std::string & text, unsigned int line, unsigned int column);
    Token(float value, unsigned int line, unsigned int column);

    Kind kind() const;

    /**
     * The name of an identifier or a keyword, the spelling of an operator or a
     * punctuation mark, or the contents of a string with its escapes resolved and its
     * quotes gone. Empty for a number and for END.
     **/
    const std::string & text() const;

    /**
     * The value of a number. SL has no integer type, so every numeric literal is one of
     * these however it was written.
     **/
    float value() const;

    /**
     * Where the token started, counting from one. A parse error that does not say where
     * is most of the cost of a parse error.
     **/
    unsigned int line() const;
    unsigned int column() const;

 private:
    Kind kind_ = Kind::END;
    std::string text_;
    float value_ = 0.0f;
    unsigned int line_ = 0;
    unsigned int column_ = 0;
};

};  // namespace v3d::render::offline::sl
