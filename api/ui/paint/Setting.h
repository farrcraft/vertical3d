/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include "Type.h"

namespace v3d::ui::paint {

/**
 * Text set in another weight or at another size than the body text.
 **/
struct Setting final {
    Type type;             /**< what it is set in. Unset sets the text as the body text is set **/
    float scale { 1.0F };  /**< how large it is against the body text **/
};

};  // namespace v3d::ui::paint
