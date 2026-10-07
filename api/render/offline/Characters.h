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
    /** The column of the last character consumed, zero before a line's first character. **/
    unsigned int column() const;

    /**
     * The rest of a quoted string whose opening quote has been consumed, up to and including
     * the closing quote. Its escapes are resolved: the standard C escapes, up to three octal
     * digits, and any other escaped character as itself.
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

};  // namespace v3d::render::offline
