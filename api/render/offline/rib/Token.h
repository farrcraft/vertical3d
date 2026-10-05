/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace v3d::render::offline::rib {

/**
 * One lexical unit of a RIB stream.
 *
 * An ASCII RIB file holds nothing else: a request name, a quoted string, a number, and
 * the two array delimiters. That is what makes recovery from an unrecognised request
 * possible without knowing its arity - only a string, a number or an array can be an
 * argument, so the next identifier begins the next request whether or not either one is
 * recognised.
 **/
class Token final {
 public:
    enum class Kind {
        END,
        IDENTIFIER,
        STRING,
        NUMBER,
        ARRAY_BEGIN,
        ARRAY_END
    };

    Token();
    Token(Kind kind, unsigned int line, unsigned int column);
    Token(Kind kind, const std::string & text, unsigned int line, unsigned int column);
    Token(float value, unsigned int line, unsigned int column);

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

};  // namespace v3d::render::offline::rib
