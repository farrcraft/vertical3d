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
 * KeyState keeps track of the current state of all keys based on all of the key up/down
 * events that we've seen.
 *
 * held() reports the current state; pressed() and released() report changes since the last
 * flush. A key pressed and released between two flushes returns true from both pressed() and
 * released(), but was never held when anything checked, so polling held() alone misses it.
 * The loop clears the edges once per frame, not the reader; see engine::Engine::eventLoop.
 **/
class KeyState final {
 public:
    /**
     * Is a key currently held?
     *
     * @return bool
     **/
    bool held(std::string_view c) const;

    /**
     * Did a key go down since the last flush?
     *
     * @param c the key's name
     **/
    bool pressed(std::string_view c) const;

    /**
     * Did a key come up since the last flush?
     *
     * @param c the key's name
     **/
    bool released(std::string_view c) const;

    /**
     * Forget this frame's edges, leaving what is held alone. Called once per frame by the
     * loop, after everything that reads them has run.
     **/
    void flush();

    /**
     * Toggle the state of the key
     *
     * @param c The key being toggled
     *
     * @return bool true if the resulting state is a held key
     **/
    bool operator() (const std::string& c);

 private:
    std::vector<std::string> keys_;
    /**< what went down since the last flush, and what came up **/
    std::vector<std::string> pressed_;
    std::vector<std::string> released_;
};

};  // namespace v3d::input
