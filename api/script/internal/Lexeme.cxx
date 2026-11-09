/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Lexeme.h"

#include <string_view>

namespace v3d::script::internal {

/**
 **/
bool Lexeme::is(std::string_view written) const noexcept {
    return (kind == Token::Punctuator || kind == Token::Identifier) && text == written;
}

};  // namespace v3d::script::internal
