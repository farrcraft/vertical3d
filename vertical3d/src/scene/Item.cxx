/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Item.h"

namespace v3d::editor {

/**
 **/
Item::Item() :
    selected_(false) {
}

/**
 **/
Item::~Item() {
}

/**
 **/
bool Item::selected() const noexcept {
    return selected_;
}

/**
 **/
void Item::selected(bool selected) noexcept {
    selected_ = selected;
}

};  // namespace v3d::editor
