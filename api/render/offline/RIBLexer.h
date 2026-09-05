/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <iosfwd>
#include <string>

namespace v3d::render::offline {

    /**
     * One lexical unit of a RIB stream.
     *
     * An ASCII RIB file holds nothing else: a request name, a quoted string, a number, and
     * the two array delimiters. That is what makes recovery from an unrecognised request
     * possible without knowing its arity - only a string, a number or an array can be an
     * argument, so the next identifier begins the next request whether or not either one is
     * recognised.
     **/
    class RIBToken final {
     public:
        enum class Kind {
            END,
            IDENTIFIER,
            STRING,
            NUMBER,
            ARRAY_BEGIN,
            ARRAY_END
        };

        RIBToken();
        RIBToken(Kind kind, unsigned int line, unsigned int column);
        RIBToken(Kind kind, const std::string & text, unsigned int line, unsigned int column);
        RIBToken(float value, unsigned int line, unsigned int column);

        Kind kind() const;
        /**
         * The name of an identifier or the contents of a string, with the escapes resolved
         * and the quotes gone. Empty for every other kind.
         **/
        const std::string & text() const;
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
    class RIBLexer final {
     public:
        /**
         * Binary and gzipped RIB are detected here, by the bytes the stream opens with, and
         * leave error() set with next() at END. Both parse into nonsense otherwise.
         *
         * The stream is read from as tokens are asked for and has to outlive the lexer.
         **/
        explicit RIBLexer(std::istream & stream);

        /**
         * The next token, consuming it. END once the stream is spent or an error is set.
         **/
        RIBToken next();

        /**
         * The next token, leaving it for next() to return.
         **/
        const RIBToken & peek();

        /**
         * What went wrong, or empty. A lexer with an error set yields nothing but END.
         **/
        const std::string & error() const;

     private:
        RIBToken scan();
        RIBToken scanString(unsigned int line, unsigned int column);
        RIBToken scanNumber(unsigned int line, unsigned int column);
        RIBToken scanIdentifier(unsigned int line, unsigned int column);

        /**
         * Consume one character, tracking the position.
         **/
        int get();
        int look();
        void skipSpace();
        RIBToken fail(const std::string & message, unsigned int line, unsigned int column);

        std::istream & stream_;
        RIBToken peeked_;
        bool peeking_ = false;
        std::string error_;
        unsigned int line_ = 1;
        unsigned int column_ = 0;
    };

};  // namespace v3d::render::offline
