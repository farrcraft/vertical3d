/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <string_view>

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

/**
 * A source name split into its key and the modifiers held with it.
 *
 * A binding names a chord by putting modifier words before the key, joined by "+": "ctrl+z",
 * "ctrl+shift+z", "alt+left". The words are ctrl, shift and alt, and each matches either side
 * of the keyboard. A name with no "+" is a key alone. A name with a word before the last "+"
 * that is not one of the three is a key alone too, and names a key no device sends.
 **/
struct Chord final {
    std::string key;
    Modifiers modifiers;
};

/**
 * Split a source name into its key and its modifiers.
 **/
Chord chord(std::string_view name);

/**
 * A source name as a menu or a tooltip shows it: each modifier and the key capitalised and
 * joined by "+", with underscores read as spaces. "ctrl+shift+z" is "Ctrl+Shift+Z", and
 * "arrow_up" is "Arrow Up".
 **/
std::string printed(std::string_view name);

};  // namespace v3d::event
