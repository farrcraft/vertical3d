/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace v3d::input {

/**
 * Which keys are held, built from every key down and key up event the keyboard has handled.
 *
 * held() reports the current state. pressed() and released() report the edges: the changes
 * since the last flush. A key pressed and released between two flushes returns true from both
 * pressed() and released(). It was never held when anything checked, so polling held() alone
 * misses it. engine::Engine::eventLoop() clears the edges once per frame, so a reader does not.
 **/
class KeyState final {
 public:
    /**
     * Is a key currently held?
     *
     * @return bool
     **/
    bool held(std::string_view key) const;

    /**
     * Did a key go down since the last flush?
     *
     * @param key the key's name
     **/
    bool pressed(std::string_view key) const;

    /**
     * Did a key come up since the last flush?
     *
     * @param key the key's name
     **/
    bool released(std::string_view key) const;

    /**
     * Forget this frame's edges, leaving what is held alone. Called once per frame by the
     * loop, after everything that reads them has run.
     **/
    void flush();

    /**
     * Toggle the state of the key
     *
     * @param key The key being toggled
     *
     * @return bool true if the resulting state is a held key
     **/
    bool operator() (const std::string& key);

 private:
    std::vector<std::string> keys_;
    /**< what went down since the last flush, and what came up **/
    std::vector<std::string> pressed_;
    std::vector<std::string> released_;
};

};  // namespace v3d::input
