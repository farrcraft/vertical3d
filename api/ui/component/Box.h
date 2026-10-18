/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Component.h>

namespace v3d::ui::component {

/**
 * A component that arranges what it holds in a line.
 *
 * It holds any number of children: an objectives list, a party of heroes, a row of loot.
 * The box only says how its children are arranged. The renderer places them as it draws,
 * in the same pass that lays out every other box.
 *
 * A child's own layout sizes it along the line. Across the line, a child is the size it
 * asks for, or the box's whole extent when the box stretches.
 *
 * A box that wraps starts a new line where the next child would not fit, so a grid of slots
 * is one box, not a box of rows. Its size across the lines is the depth of the lines its
 * children fill.
 **/
class Box : public Component {
 public:
    ~Box() = default;

    /**
     * The gap between one child and the next, in pixels.
     **/
    void spacing(float gap);
    float spacing() const noexcept;

    /**
     * Whether each child is widened to the box's extent across the line, so that the rows
     * of a list are all one width. False by default.
     **/
    void stretch(bool fill);
    bool stretch() const noexcept;

    /**
     * Whether a child that would run past the end of the line starts a new one, spaced from
     * the last by the same gap. A box that wraps does not stretch. False by default.
     **/
    void wrap(bool flow);
    bool wrap() const noexcept;

 protected:
    explicit Box(Type type);

 private:
    float spacing_;
    bool stretch_;
    bool wrap_;
};

};  // namespace v3d::ui::component
