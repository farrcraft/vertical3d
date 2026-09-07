/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Component.h"

namespace v3d::ui::component {

/**
 * A component that arranges what it holds in a line.
 *
 * However many children it was given: an objectives list, a party of heroes, a row of
 * loot. The arithmetic is the renderer's, per ADR-0034 - the box says how it is arranged
 * and the draw walk does it, in the same pass that resolves every other box.
 *
 * A child's own layout is what sizes it along the line; across the line it is either the
 * size it asks for or, when the box stretches, the whole width of the box.
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
     * Whether a child is widened to the box across the direction it flows in, which is
     * what makes a list of rows read as rows. False by default.
     **/
    void stretch(bool fill);
    bool stretch() const noexcept;

 protected:
    explicit Box(Type type);

 private:
    float spacing_;
    bool stretch_;
};

};  // namespace v3d::ui::component
