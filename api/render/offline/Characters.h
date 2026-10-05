/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <istream>
#include <string>

namespace v3d::render::offline {

/**
 * The character input shared by the RIB and SL lexers: a stream read a character at a time
 * with its position kept, the character classes, and how a quoted string's escapes decode.
 * Each lexer builds its own tokens on top of this class.
 **/
class Characters final {
 public:
    /** What get() and look() return once the stream is exhausted. **/
    static const int END = -1;

    /**
     * The stream is read as characters are requested, and must outlive this object.
     **/
    explicit Characters(std::istream & stream);

    /**
     * Consume one character, tracking the line and column.
     **/
    int get();

    /**
     * The next character, leaving it to be consumed.
     **/
    int look();

    /**
     * Put back the character get() just returned, one deep.
     *
     * The stream's own putback is not used because it fails once the stream has reached its
     * end. A lexer that looks one character past a trailing character reaches that state.
     **/
    void unget(int c);

    /** The line of the last character consumed, counting from one. **/
    unsigned int line() const;
    /** The column of the last character consumed, which is zero before the first character of a line. **/
    unsigned int column() const;

    /**
     * The rest of a quoted string whose opening quote has been consumed, with its escapes
     * resolved, up to and including the closing quote: the standard C escapes, up to three
     * octal digits, and any other escaped character as itself.
     *
     * @param lineEnds whether a newline ends the string unterminated rather than being part
     *        of it
     * @return false when the string was not terminated
     **/
    bool quoted(bool lineEnds, std::string* text);

    static bool digit(int c);
    static bool octal(int c);
    /** A letter or an underscore: the characters a name may start with. **/
    static bool alpha(int c);
    static bool space(int c);

    /**
     * Formats a position as " at line 3, column 7". Every lexer and parser error message ends
     * with it.
     **/
    static std::string position(unsigned int line, unsigned int column);

 private:
    std::istream & stream_;
    int pushback_ = 0;
    bool pushed_ = false;
    unsigned int line_ = 1;
    unsigned int column_ = 0;
};

/**
 * One lexical unit, whichever language it came from: what kind it is, its text or its value,
 * and where it started. Each language says what its kinds are.
 **/
template <typename Kind>
class Lexeme {
 public:
    Lexeme() = default;
    Lexeme(Kind kind, unsigned int line, unsigned int column) :
        kind_(kind), line_(line), column_(column) {
    }
    Lexeme(Kind kind, const std::string & text, unsigned int line, unsigned int column) :
        kind_(kind), text_(text), line_(line), column_(column) {
    }
    /** A number. Only a number lexeme carries a value. **/
    Lexeme(Kind number, float value, unsigned int line, unsigned int column) :
        kind_(number), value_(value), line_(line), column_(column) {
    }

    Kind kind() const {
        return kind_;
    }

    /**
     * The name of an identifier, the spelling of a symbol, or the contents of a string with
     * its escapes resolved and its quotes gone. Empty for a number.
     **/
    const std::string & text() const {
        return text_;
    }

    float value() const {
        return value_;
    }

    /**
     * Where the lexeme started, counting from one. Parse errors report this position.
     **/
    unsigned int line() const {
        return line_;
    }
    unsigned int column() const {
        return column_;
    }

 private:
    Kind kind_ = Kind::END;
    std::string text_;
    float value_ = 0.0f;
    unsigned int line_ = 0;
    unsigned int column_ = 0;
};

};  // namespace v3d::render::offline
