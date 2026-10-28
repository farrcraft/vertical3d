/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace v3d::font {

/**
 * The code point at an index of a UTF-8 string, and the index moved past it.
 *
 * A byte that does not begin a well formed sequence gives U+FFFD, the replacement character,
 * and the index moves on by that one byte. Overlong forms and surrogates count as malformed.
 *
 * @param index where to read, which must be before the end, moved to the next character
 **/
char32_t next(std::string_view utf8, std::size_t* index);

/**
 * The code points a UTF-8 string holds, one per character, decoded as next() decodes them.
 **/
std::u32string decode(std::string_view utf8);

};  // namespace v3d::font
