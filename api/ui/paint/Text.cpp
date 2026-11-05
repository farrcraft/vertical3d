/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Text.h"

namespace v3d::ui::paint {

/**
 **/
namespace {

/** the lightest number CSS calls a medium weight, and the first this library sets semibold **/
constexpr float MEDIUM_WEIGHT = 500.0F;

};  // namespace

Weight weight(float number) noexcept {
    return number >= MEDIUM_WEIGHT ? Weight::SemiBold : Weight::Regular;
}

};  // namespace v3d::ui::paint
