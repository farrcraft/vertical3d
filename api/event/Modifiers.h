/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::event {

/**
 * The modifier keys held with a key: Ctrl, Shift and Alt, each on either side of the keyboard.
 **/
struct Modifiers final {
    bool ctrl = false;
    bool shift = false;
    bool alt = false;

    /**
     * @return how many of the three are held
     **/
    int count() const noexcept;

    /**
     * @return whether every modifier this names is among those held
     **/
    bool within(const Modifiers& held) const noexcept;

    bool operator==(const Modifiers& other) const = default;
};

};  // namespace v3d::event
