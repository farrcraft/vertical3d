/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>

#include "EffectQuad.h"

#include <glm/vec4.hpp>

namespace v3d::ui::paint::internal {

/**
 * An effect laid out in a paragraph's space: its quad moved to its glyph's pen, its place among
 * its glyph's effects, and its colour.
 **/
struct PlacedEffect final {
    EffectQuad quad;
    std::size_t layer = 0;
    glm::vec4 colour{ 1.0F };
};

};  // namespace v3d::ui::paint::internal
