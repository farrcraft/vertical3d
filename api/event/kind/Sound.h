/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <string_view>

namespace v3d::event::kind {

/**
 * A request to play a clip as a one shot, which audio::Engine listens for.
 **/
class Sound final {
 public:
    /**
     * @param clip the id the clip was filed under
     * @param pan where the sound sits between the speakers, from -1 left to 1 right, as
     *        audio::Play::pan places it. The game converts a position to a pan
     **/
    // not noexcept: clip_ is a std::string built from the view, which allocates
    Sound(const std::string_view &clip, float pan = 0.0f);
    std::string_view clip() const noexcept;
    float pan() const noexcept;

 private:
    std::string clip_;
    float pan_;
};
};  // namespace v3d::event::kind
