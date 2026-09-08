/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <iosfwd>
#include <string>

namespace v3d::render::offline {

/**
 * One lexical unit of a shading language source.
 *
 * A keyword is separated from an identifier here rather than in the parser because the
 * set is closed and small: the shader types, the data types, the storage classes, the
 * control flow, and the three lighting constructs. Everything else that looks like a name
 * is an identifier, and what it means is the symbol table's answer rather than the
 * lexer's - which is what lets a shader declare a variable named after a built-in.
 **/
class SLToken final {
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

    SLToken();
    SLToken(Kind kind, unsigned int line, unsigned int column);
    SLToken(Kind kind, const std::string & text, unsigned int line, unsigned int column);
    SLToken(float value, unsigned int line, unsigned int column);

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
class SLLexer final {
 public:
    /**
     * The stream is read from as tokens are asked for and has to outlive the lexer.
     **/
    explicit SLLexer(std::istream & stream);

    /**
     * The next token, consuming it. END once the stream is spent or an error is set.
     **/
    SLToken next();

    /**
     * The next token, leaving it for next() to return.
     **/
    const SLToken & peek();

    /**
     * What went wrong, or empty. A lexer with an error set yields nothing but END.
     **/
    const std::string & error() const;

 private:
    SLToken scan();
    SLToken scanString(unsigned int line, unsigned int column);
    SLToken scanNumber(unsigned int line, unsigned int column);
    SLToken scanName(unsigned int line, unsigned int column);
    SLToken scanOperator(unsigned int line, unsigned int column);

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
    SLToken fail(const std::string & message, unsigned int line, unsigned int column);

    std::istream & stream_;
    SLToken peeked_;
    bool peeking_ = false;
    int pushback_ = 0;
    bool pushed_ = false;
    std::string error_;
    unsigned int line_ = 1;
    unsigned int column_ = 0;
};

};  // namespace v3d::render::offline
