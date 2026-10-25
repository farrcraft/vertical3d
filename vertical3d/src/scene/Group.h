/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Item.h"

namespace v3d::editor {

/**
 * An item that holds nothing of its own, and places the items beneath it.
 *
 * Any item may be a parent, so a group is only needed where nothing else is to be the parent.
 * It is drawn and picked as a small box at its origin, since it has no geometry to click.
 **/
class Group final : public Item {
 public:
    static constexpr Kind KIND = Kind::Group;

    Group();

    Kind kind() const noexcept override;
};

};  // namespace v3d::editor
