/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Box.h"

namespace v3d::ui::component {

/**
 * A box whose children run across it, left to right.
 **/
class HorizontalBox : public Box {
 public:
    HorizontalBox();
    ~HorizontalBox() = default;
};

};  // namespace v3d::ui::component
