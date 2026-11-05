/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace v3d::script {

/**
 * Where a script could not be stripped, and why.
 **/
struct Problem final {
    std::size_t line = 0;       /**< from 1 **/
    std::size_t column = 0;     /**< from 1, in bytes **/
    std::string message;
};

/**
 * TypeScript with its types taken out, or why they could not be.
 **/
struct Stripped final {
    std::string code;                   /**< the JavaScript left, the source's length and lines **/
    std::optional<Problem> problem;     /**< set, with no code, when the source was refused **/
};

/**
 * Take the types out of TypeScript, and leave JavaScript to run.
 *
 * Each annotation, type argument, assertion and declaration of types is blanked with spaces,
 * and its line breaks are kept, so every line and column of the JavaScript is where it was. A
 * return type that runs onto another line takes the ) before it to its end, since => may not
 * follow a line break. Syntax with no JavaScript to keep when its types go is refused: an
 * enum, a namespace, a parameter property, a type assertion written `<T>value`, import = and
 * export =. A source that does not read as TypeScript is refused too.
 *
 * The types are never checked.
 **/
Stripped strip(std::string_view source);

};  // namespace v3d::script
