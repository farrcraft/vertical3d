/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>
#include <vector>

#include "Sprite.h"

namespace v3d::image {

/**
 * What unpack() cut: one sprite for each placement, or the reason there are none.
 **/
class Unpacking final {
 public:
    /** One for each placement, in the order the placements were given. **/
    std::vector<Sprite> sprites;
    /** Empty when every placement was cut. **/
    std::string refusal;
};

};  // namespace v3d::image
