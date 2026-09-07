/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Box.h"

namespace v3d::ui::component {

/**
 * A box whose children run down it, top to bottom.
 **/
class VerticalBox : public Box {
 public:
    VerticalBox();
    ~VerticalBox() = default;
};

};  // namespace v3d::ui::component
