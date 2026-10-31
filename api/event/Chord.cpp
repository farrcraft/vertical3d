/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Chord.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace v3d::event {

namespace {

/**
 * A word with its first letter, and each letter after a space, in capitals.
 **/
std::string capitalised(std::string_view word) {
    std::string out(word);
    bool start = true;
    for (char& letter : out) {
        if (letter == '_') {
            letter = ' ';
            start = true;
        } else if (start) {
            if (letter >= 'a' && letter <= 'z') {
                letter = static_cast<char>(letter - 'a' + 'A');
            }
            start = false;
        }
    }
    return out;
}

};  // namespace

int Modifiers::count() const noexcept {
    return (ctrl ? 1 : 0) + (shift ? 1 : 0) + (alt ? 1 : 0);
}

bool Modifiers::within(const Modifiers& held) const noexcept {
    return (!ctrl || held.ctrl) && (!shift || held.shift) && (!alt || held.alt);
}

Chord chord(std::string_view name) {
    const std::size_t last = name.rfind('+');
    if (last == std::string_view::npos || last + 1 == name.size()) {
        return Chord{.key = std::string(name), .modifiers = Modifiers()};
    }

    Chord found{.key = std::string(name.substr(last + 1)), .modifiers = Modifiers()};
    // every word before the key, an empty one included, has to be a modifier
    std::string_view words = name.substr(0, last);
    while (true) {
        const std::size_t plus = words.find('+');
        const std::string_view word = words.substr(0, plus);
        if (word == "ctrl") {
            found.modifiers.ctrl = true;
        } else if (word == "shift") {
            found.modifiers.shift = true;
        } else if (word == "alt") {
            found.modifiers.alt = true;
        } else {
            // not a chord after all, so the whole name is the key
            return Chord{.key = std::string(name), .modifiers = Modifiers()};
        }
        if (plus == std::string_view::npos) {
            return found;
        }
        words = words.substr(plus + 1);
    }
}

std::string printed(std::string_view name) {
    const Chord split = chord(name);
    std::string out;
    if (split.modifiers.ctrl) {
        out += "Ctrl+";
    }
    if (split.modifiers.shift) {
        out += "Shift+";
    }
    if (split.modifiers.alt) {
        out += "Alt+";
    }
    return out + capitalised(split.key);
}

};  // namespace v3d::event
