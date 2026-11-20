/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace v3d::script::internal {

/**
 * What a token of TypeScript is.
 **/
enum class Token : std::uint8_t {
    End,            /**< past the last token **/
    Identifier,     /**< a name or a keyword **/
    Private,        /**< a private name, such as #count **/
    Number,
    String,
    Template,       /**< a whole template without a substitution, or the end of one after its last **/
    TemplateHead,   /**< a template up to the ${ that opens a substitution **/
    TemplateMiddle, /**< a template from a substitution's } to the ${ that opens the next **/
    Regex,
    Punctuator,
    Unclosed,       /**< a comment, string, template or regular expression that never ends **/
};

/**
 * One token, and where it is in the source.
 **/
struct Lexeme final {
    Token kind = Token::End;
    std::size_t begin = 0;
    std::size_t end = 0;
    std::string_view text;
    bool newline = false;   /**< whether a line break lies between it and the token before **/

    /**
     * @return whether it is the punctuator or the identifier written as some text
     **/
    bool is(std::string_view written) const noexcept;
};

};  // namespace v3d::script::internal
